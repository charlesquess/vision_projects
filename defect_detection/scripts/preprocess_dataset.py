"""
PCB 数据集预处理 — 滑窗裁剪 + 旋转增强 + YOLO 格式转换

用法:
  conda run -n yolov5 python scripts/preprocess_dataset.py

流程:
  1. 遍历 PCB_DATASET 所有图片和 XML 标注
  2. 固定步长 512 滑窗裁剪 640x640（20% 重叠）
  3. 边界不足处保留，填充黑色到 640x640
  4. 以 50% 概率对裁剪块做随机旋转 (-10°~+10°)，标注框同步旋转
  5. 保存为 YOLO 格式 (images + labels)
  6. 生成 train/val 划分和 dataset.yaml
"""
import random
import xml.etree.ElementTree as ET
from pathlib import Path
import cv2
import numpy as np

# ── 配置 ───────────────────────────────────────────────────────
PCB_ROOT = Path(__file__).resolve().parent.parent.parent / "PCB_DATASET"
OUT_ROOT = Path(__file__).resolve().parent.parent / "data" / "yolo_dataset"

CROP_SIZE = 640            # 裁剪尺寸
STRIDE = 512               # 固定步长 (640 * 0.8 = 20% 重叠)
ROTATE_PROB = 0.5          # 旋转增强概率
ROTATE_RANGE = (-10, 10)   # 旋转角度范围 (度)

VAL_SPLIT = 0.15
RANDOM_SEED = 42

CLASSES = [
    "missing_hole", "mouse_bite", "open_circuit",
    "short", "spur", "spurious_copper",
]
CLASS_TO_ID = {n: i for i, n in enumerate(CLASSES)}


def parse_voc_xml(xml_path: Path):
    tree = ET.parse(xml_path)
    root = tree.getroot()
    size = root.find("size")
    img_w = int(size.find("width").text)
    img_h = int(size.find("height").text)
    objects = []
    for obj in root.iter("object"):
        name = obj.find("name").text.lower()
        if name not in CLASS_TO_ID:
            continue
        bbox = obj.find("bndbox")
        objects.append({
            "class_id": CLASS_TO_ID[name],
            "xmin": int(float(bbox.find("xmin").text)),
            "ymin": int(float(bbox.find("ymin").text)),
            "xmax": int(float(bbox.find("xmax").text)),
            "ymax": int(float(bbox.find("ymax").text)),
        })
    return img_w, img_h, objects


def sliding_windows_fixed(img_w: int, img_h: int):
    windows = []
    y = 0
    while y < img_h:
        x = 0
        while x < img_w:
            windows.append((x, y))
            x += STRIDE
        y += STRIDE
    return windows


def crop_and_pad(img, win_x, win_y):
    h, w = img.shape[:2]
    x1, y1 = win_x, win_y
    x2, y2 = min(win_x + CROP_SIZE, w), min(win_y + CROP_SIZE, h)
    crop = img[y1:y2, x1:x2]
    actual_h, actual_w = crop.shape[:2]
    if actual_h < CROP_SIZE or actual_w < CROP_SIZE:
        padded = np.zeros((CROP_SIZE, CROP_SIZE, 3), dtype=np.uint8)
        padded[:actual_h, :actual_w] = crop
        return padded, actual_h, actual_w
    return crop, actual_h, actual_w


def crop_object_in_window(obj, win_x, win_y, crop_h, crop_w):
    xmin = max(obj["xmin"], win_x)
    ymin = max(obj["ymin"], win_y)
    xmax = min(obj["xmax"], win_x + crop_w)
    ymax = min(obj["ymax"], win_y + crop_h)
    if xmin >= xmax or ymin >= ymax:
        return None
    return {
        "class_id": obj["class_id"],
        "xmin": xmin - win_x,
        "ymin": ymin - win_y,
        "xmax": xmax - win_x,
        "ymax": ymax - win_y,
    }


def rotate_image_and_boxes(img, boxes, angle):
    """
    围绕图像中心旋转 angle 度，保持输出尺寸 640x640。
    四个角用背景色 (128,128,128) 填充。
    boxes: list of {class_id, xmin, ymin, xmax, ymax} (坐标在 0~639 范围内)
    返回 (旋转后图像, 旋转后 boxes)
    """
    h, w = img.shape[:2]
    cx, cy = w / 2.0, h / 2.0
    M = cv2.getRotationMatrix2D((cx, cy), angle, 1.0)
    rotated = cv2.warpAffine(img, M, (w, h), borderValue=(128, 128, 128))

    new_boxes = []
    for box in boxes:
        # 框的四个角
        corners = np.array([
            [box["xmin"], box["ymin"]],
            [box["xmax"], box["ymin"]],
            [box["xmax"], box["ymax"]],
            [box["xmin"], box["ymax"]],
        ], dtype=np.float32)

        # 应用旋转矩阵
        ones = np.ones((4, 1))
        corners_h = np.hstack([corners, ones])
        rotated_corners = (M @ corners_h.T).T

        # 取旋转后外接矩形
        xs = rotated_corners[:, 0]
        ys = rotated_corners[:, 1]
        xmin = max(0, np.clip(xs.min(), 0, w - 1))
        xmax = min(w, np.clip(xs.max(), 0, w))
        ymin = max(0, np.clip(ys.min(), 0, h - 1))
        ymax = min(h, np.clip(ys.max(), 0, h))

        if xmin >= xmax or ymin >= ymax:
            continue

        new_boxes.append({
            "class_id": box["class_id"],
            "xmin": int(round(xmin)),
            "ymin": int(round(ymin)),
            "xmax": int(round(xmax)),
            "ymax": int(round(ymax)),
        })

    return rotated, new_boxes


def to_yolo(rel_obj, img_size):
    cx = (rel_obj["xmin"] + rel_obj["xmax"]) / 2.0 / img_size
    cy = (rel_obj["ymin"] + rel_obj["ymax"]) / 2.0 / img_size
    w_ = (rel_obj["xmax"] - rel_obj["xmin"]) / img_size
    h_ = (rel_obj["ymax"] - rel_obj["ymin"]) / img_size
    return f"{rel_obj['class_id']} {cx:.6f} {cy:.6f} {w_:.6f} {h_:.6f}"


def main():
    random.seed(RANDOM_SEED)

    # 收集图片+XML对
    pairs = []
    for xml_path in sorted((PCB_ROOT / "Annotations").rglob("*.xml")):
        rel = xml_path.relative_to(PCB_ROOT / "Annotations").parts
        img_path = PCB_ROOT / "images" / rel[0] / (xml_path.stem + ".jpg")
        if img_path.exists():
            pairs.append((img_path, xml_path))

    print(f"找到 {len(pairs)} 对 图片+标注")

    # 输出目录
    dirs = {}
    for d in ("images", "labels"):
        for s in ("train", "val"):
            p = OUT_ROOT / d / s
            p.mkdir(parents=True, exist_ok=True)
            dirs[f"{d}/{s}"] = p

    total_crops = 0
    crops_with_obj = 0
    rotated_count = 0
    records = []

    for img_path, xml_path in pairs:
        img_w, img_h, objects = parse_voc_xml(xml_path)
        windows = sliding_windows_fixed(img_w, img_h)
        img = cv2.imread(str(img_path))
        if img is None:
            continue

        for (win_x, win_y) in windows:
            total_crops += 1

            # 裁剪并填充
            crop, ch, cw = crop_and_pad(img, win_x, win_y)

            # 找窗口内的目标
            local_objs = []
            for obj in objects:
                loc = crop_object_in_window(obj, win_x, win_y, ch, cw)
                if loc is not None:
                    local_objs.append(loc)

            if not local_objs:
                continue

            crops_with_obj += 1
            stem = f"{img_path.stem}_x{win_x}y{win_y}"
            split = "val" if random.random() < VAL_SPLIT else "train"

            # 旋转增强（仅训练集）
            do_rotate = (split == "train" and random.random() < ROTATE_PROB)
            if do_rotate:
                angle = random.uniform(*ROTATE_RANGE)
                crop, local_objs = rotate_image_and_boxes(crop, local_objs, angle)
                if not local_objs:   # 旋转后所有框都出界，丢弃
                    continue
                rotated_count += 1
                stem += f"_rot{angle:.0f}"

            cv2.imwrite(str(dirs[f"images/{split}"] / f"{stem}.jpg"), crop)

            label_lines = [to_yolo(o, CROP_SIZE) for o in local_objs]
            (dirs[f"labels/{split}"] / f"{stem}.txt").write_text("\n".join(label_lines))
            records.append((stem, split))

    # dataset.yaml
    nc = len(CLASSES)
    yaml_path = OUT_ROOT / "dataset.yaml"
    yaml_content = (
        f"# PCB 缺陷检测数据集 (滑窗裁剪 {CROP_SIZE}x{CROP_SIZE}, 步长 {STRIDE}, 重叠 {1-STRIDE/CROP_SIZE:.0%})\n"
        f"# 旋转增强: 概率 {ROTATE_PROB:.0%}, 范围 {ROTATE_RANGE[0]}~{ROTATE_RANGE[1]} 度\n"
        f"train: {(OUT_ROOT / 'images' / 'train').as_posix()}\n"
        f"val:   {(OUT_ROOT / 'images' / 'val').as_posix()}\n"
        f"\nnc: {nc}\nnames:\n"
    )
    for i, name in enumerate(CLASSES):
        yaml_content += f"  {i}: {name}\n"
    yaml_path.write_text(yaml_content)

    train_count = len([r for r in records if r[1] == "train"])
    val_count = len([r for r in records if r[1] == "val"])

    print(f"\n{'='*55}")
    print(f"  裁剪尺寸     : {CROP_SIZE}x{CROP_SIZE}")
    print(f"  固定步长     : {STRIDE} ({1-STRIDE/CROP_SIZE:.0%} 重叠)")
    print(f"  边界处理     : 保留 + 填充黑色到 640x640")
    print(f"  旋转增强     : 概率 {ROTATE_PROB:.0%}, 范围 {ROTATE_RANGE[0]}~{ROTATE_RANGE[1]} 度")
    print(f"  → 实际旋转   : {rotated_count} 块")
    print(f"  总裁剪块     : {total_crops}")
    print(f"  含目标块     : {crops_with_obj}")
    print(f"  训练集       : {train_count}")
    print(f"  验证集       : {val_count}")
    print(f"  输出目录     : {OUT_ROOT}")
    print(f"  标签文件     : {yaml_path}")
    print(f"{'='*55}")


if __name__ == "__main__":
    main()
