# -*- coding: utf-8 -*-
"""
简单摄像头测试：打开默认摄像头，实时显示画面。
按 Q 或 ESC 退出。

若报错 “The function is not implemented” 且与 imshow 相关，多半是环境里装了
opencv-python-headless（无窗口）。请改用带界面的包，见 main 中的说明。
"""
import sys
from pathlib import Path

import cv2
import numpy as np


def open_capture(index: int = 0):
    """在 Windows 上优先用 DirectShow，部分 USB 摄像头更稳定。"""
    if sys.platform == "win32":
        cap = cv2.VideoCapture(index, cv2.CAP_DSHOW)
    else:
        cap = cv2.VideoCapture(index)
    return cap


def highgui_works() -> bool:
    """检测当前 OpenCV 是否编译了 highgui（能否使用 imshow）。"""
    try:
        probe = np.zeros((10, 10, 3), dtype=np.uint8)
        cv2.imshow("__opencv_gui_probe__", probe)
        cv2.waitKey(1)
        cv2.destroyWindow("__opencv_gui_probe__")
        return True
    except cv2.error:
        try:
            cv2.destroyAllWindows()
        except cv2.error:
            pass
        return False


def print_headless_help():
    print(
        "\n当前 OpenCV 无法弹出窗口（通常为安装了 opencv-python-headless）。\n"
        "请在当前环境中执行：\n"
        "  pip uninstall opencv-python-headless -y\n"
        "  pip install opencv-python\n"
        "若同时装了 opencv-contrib-python-headless，也请卸载后安装 opencv-contrib-python。\n"
    )


def main():
    index = 0
    if len(sys.argv) > 1:
        try:
            index = int(sys.argv[1])
        except ValueError:
            print("用法: python testcamera.py [摄像头编号]，默认 0")
            sys.exit(1)

    cap = open_capture(index)
    if not cap.isOpened():
        print(f"无法打开摄像头 {index}，请检查设备、驱动或尝试编号 1、2。")
        sys.exit(1)

    w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    print(f"摄像头 {index} 已打开，分辨率约 {w}x{h}。")

    if not highgui_works():
        print_headless_help()
        ret, frame = cap.read()
        cap.release()
        if not ret or frame is None:
            print("读取帧失败。")
            sys.exit(1)
        out = Path(__file__).resolve().parent / "testcamera_snapshot.jpg"
        cv2.imwrite(str(out), frame)
        print(f"已保存一帧到: {out}，请打开图片确认摄像头画面是否正常。")
        sys.exit(0)

    print("按 Q 或 ESC 退出。")

    while True:
        ret, frame = cap.read()
        if not ret or frame is None:
            print("读取帧失败，退出。")
            break

        cv2.putText(
            frame,
            f"Cam {index}  Q/ESC quit",
            (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 0),
            2,
            cv2.LINE_AA,
        )
        try:
            cv2.imshow("testcamera", frame)
        except cv2.error as e:
            print(e)
            print_headless_help()
            break

        key = cv2.waitKey(1) & 0xFF
        if key == ord("q") or key == 27:
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
