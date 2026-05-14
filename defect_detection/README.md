# PCB Defect Detection System

基于计算机视觉的 PCB 缺陷检测系统。采用 C++ 实现核心流水线，支持 YOLOv5 ONNX 模型推理，提供从相机采集到检测结果输出的完整闭环。

## 系统架构

```
                     +─────────────────────────────────────────────+
                     │                 Main Loop                   │
                     +─────────────────────────────────────────────+
                                     │
         ┌───────────┬──────────┬────┴────┬──────────┬────────────┐
         ▼           ▼          ▼         ▼          ▼            ▼
   ┌─────────┐ ┌─────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
   │ Camera  │ │Preprocess│ │Inference│ │Postproc│ │  Comm  │ │   UI   │
   │ 采集    │ │ 预处理   │ │ ONNX   │ │ 后处理 │ │串口 PLC│ │ 界面   │
   └────┬────┘ └────┬────┘ └───┬────┘ └───┬────┘ └───┬────┘ └───┬────┘
        │           │          │          │          │          │
        ▼           ▼          ▼          ▼          ▼          ▼
   ┌─────────────────────────────────────────────────────────────────┐
   │                       Utils / Statistics                        │
   │          (Config, Logger, Timer, Statistician)                  │
   └─────────────────────────────────────────────────────────────────┘
```

### 数据流

```
Camera → Preprocess → Inference → Postprocess → UI Display
                         │                          │
                         ▼                          ▼
                   Statistics ←───────────────── Serial/PLC
```

## 目录结构

```
defect_detection/
├── 3rdparty/            # 第三方库 (serialib, onnxruntime, etc.)
├── include/             # 头文件
│   ├── camera/          #   相机采集接口
│   ├── preprocess/      #   图像预处理接口
│   ├── inference/       #   推理引擎接口
│   ├── postprocess/     #   后处理接口
│   ├── communication/   #   串口通信接口
│   ├── statistics/      #   统计模块接口
│   ├── ui/              #   界面模块接口
│   └── utils/           #   工具类 (Config, Logger, Timer, Types)
├── src/                 # 源文件
│   ├── camera/          #   相机采集实现
│   ├── preprocess/      #   图像预处理实现
│   ├── inference/       #   推理引擎实现
│   ├── postprocess/     #   后处理实现
│   ├── communication/   #   串口通信实现
│   ├── statistics/      #   统计模块实现
│   ├── ui/              #   界面模块实现
│   ├── utils/           #   工具类实现
│   └── main.cpp         # 程序入口
├── config/              # 配置文件 (config.yaml)
├── models/              # ONNX 模型文件
├── logs/                # 日志文件 & 统计报表
├── data/                # 测试数据 (含 camera_snapshot.jpg)
├── scripts/             # 辅助脚本 (test_camera.py)
├── build.ps1            # 一键编译脚本 (VS 2019 BuildTools)
├── CMakeLists.txt       # 构建配置
├── .gitignore
└── README.md
```

## 模块说明

### Camera — 相机采集

- USB 相机采集 (OpenCV VideoCapture)，自动探测 DShow / MSMF 后端
- 曝光/增益/分辨率/FPS 参数控制，通用 `set(prop_id, value)` 透传
- **帧超时检测** — 可配置 `timeout_ms`，超时返回 false 并计数丢帧
- **自动重连** — 连续丢帧超过 `retry_count` 自动 `reopen()`
- **FPS 监控** — `measured_fps()` 实时返回 1 秒滑动窗口帧率
- **帧健康检查** — `frame_healthy()` 检测空帧/全黑/全白等异常
- **ROI 裁剪** — `set_roi()` 采集后自动裁剪区域
- **快照保存** — `save_snapshot(path)` 单帧存盘
- 设备枚举 `list_available_devices()` / 设备探测 `probe_device()`

### Preprocess — 图像预处理

- 缩放、归一化、直方图均衡
- 去噪 (Non-local Means)
- 仿射变换 (校正定位)
- 颜色空间转换
- 可组合的预处理流水线

### Inference — ONNX 推理引擎

- 基于 ONNX Runtime (支持 CPU / CUDA GPU)
- 批量推理
- 模型元信息查询
- 预留 TensorRT 扩展接口

### Postprocess — 后处理

- YOLOv5 原始输出解析
- 置信度过滤
- Non-Maximum Suppression (NMS)
- 检测结果绘制

### Communication — 串口通信

- RS232/485 串口协议
- 支持 Windows (CreateFile) / Linux (termios)
- 可配置波特率、校验位、数据位、停止位
- 异步接收回调

### Statistics — 数据统计

- 检测计数 (总数/良品/缺陷)
- 良率计算
- 缺陷类型分布
- 平均推理耗时统计
- CSV/JSON 报表导出

### UI — 人机界面

- OpenCV highgui 窗口显示
- 实时 FPS 与检测数据叠加
- 检测框与标签绘制
- 键盘事件处理

## 依赖项

| 依赖 | 版本要求 | 用途 |
|------|---------|------|
| OpenCV | >= 4.5 | 图像处理、相机采集、UI |
| ONNX Runtime | >= 1.12 | 模型推理 |
| spdlog (可选) | >= 1.10 | 高性能日志 |
| yaml-cpp (可选) | >= 0.7 | YAML 配置解析 |
| CMake | >= 3.16 | 构建系统 |
| C++ Compiler | C++17 | MSVC / GCC / Clang |

## 构建

### 环境要求

| 依赖 | 版本 | 本机路径 |
|------|------|---------|
| Visual Studio 2019 BuildTools | MSVC 14.29+ | `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools` |
| OpenCV | >= 4.5 | `C:\Opencv2\opencv\build` (4.13.0) |
| CMake | >= 3.16 | VS 自带的 CMake |
| ONNX Runtime (可选) | >= 1.12 | — |

### 一键编译 (本机)

```powershell
cd defect_detection
.\build.ps1
# 或绕过执行策略:
powershell -ExecutionPolicy Bypass -File build.ps1
```

### 手动编译

```bash
# 配置
cmake -B build -G "Visual Studio 16 2019" -A x64 \
    -DOpenCV_DIR=C:/Opencv2/opencv/build

# 编译
cmake --build build --config Release

# 运行（需要将 OpenCV DLL 加入 PATH）
$env:Path = "C:\Opencv2\opencv\build\x64\vc16\bin;$env:Path"
.\build\Release\defect_detection.exe config\config.yaml
```

## 配置

所有运行参数通过 `config/config.yaml` 管理，详见文件内注释。

主要配置项：

| 配置 | 说明 | 默认值 |
|------|------|--------|
| `camera.device_id` | 相机设备号 | `0` |
| `camera.width / height` | 采集分辨率 | `640x480` |
| `camera.fps` | 目标帧率 | `30` |
| `camera.exposure` | 曝光 (μs, -1=自动) | `-1` |
| `camera.gain` | 增益 (dB, -1=自动) | `-1` |
| `camera.backend` | 后端: AutoDetect/DShow/MSMF | `DShow` |
| `camera.timeout_ms` | 帧读取超时 | `3000` |
| `camera.auto_reconnect` | 丢帧自动重连 | `true` |
| `model.path` | ONNX 模型路径 | `models/model.onnx` |
| `model.use_gpu` | 启用 GPU 推理 | `false` |
| `detection.confidence_threshold` | 置信度阈值 | `0.5` |
| `detection.nms_threshold` | NMS IoU 阈值 | `0.45` |
| `serial.port` | 串口号 | `COM1` |
| `serial.baudrate` | 波特率 | `9600` |
| `serial.parity` | 校验位 (N/E/O) | `N` |
| `ui.window_width / window_height` | 显示窗口尺寸 | `1280x720` |

## 测试

```bash
# 摄像头测试 (Python, 需先激活 conda yolov5 环境)
conda run -n yolov5 python scripts/test_camera.py [设备号]
```

测试内容：设备枚举 → 后端探测 → 连续帧采集 → 健康检查 → FPS 基准 → 快照 → 交互预览

## 数据集

项目配套 `PCB_DATASET/` 数据集，包含 PCB 常见缺陷图片与标注：

| 缺陷类型 | 描述 |
|---------|------|
| Spurious Copper | 多余的铜箔/铜渣 |
| Missing Hole | 缺失的孔 |
| Short | 短路 |
| Open Circuit | 开路 |
| Scratch | 划痕 |
| Other | 其他 |

数据集位于仓库同级目录 `../PCB_DATASET/`，同时提供 `yolov5-7.0/` 用于模型训练。

## 开发计划

- [x] 项目骨架与模块划分
- [x] 模块接口定义
- [x] 配置系统
- [x] 相机采集完整实现 (USB DShow/MSMF, 超时/重连/ROI/快照/FPS统计)
- [ ] ONNX Runtime 推理集成
- [ ] 串口通信底层实现
- [ ] 多线程流水线 (生产者-消费者模式)
- [ ] 批量检测性能优化
- [ ] TensorRT 推理后端
- [ ] 工业相机 SDK 集成 (Hikrobot, Basler)
- [ ] Qt 界面迁移

## 许可

MIT License
