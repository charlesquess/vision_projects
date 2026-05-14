#pragma once

#include <string>
#include <vector>
#include <opencv2/core/types.hpp>

/* 缺陷类型枚举，对应 PCB 检测中的常见缺陷 */
enum class DefectType {
    SpuriousCopper,  /* 多余铜箔/铜渣 */
    MissingHole,     /* 孔洞缺失 */
    Short,           /* 短路 */
    OpenCircuit,     /* 开路 */
    Scratch,         /* 划痕 */
    Other            /* 其他 */
};

/* 检测框结构，表示一个检测到的目标 */
struct BoundingBox {
    float x, y, width, height;  /* 框的左上角坐标和宽高 */
    float confidence;            /* 置信度 (0~1) */
    DefectType label;            /* 缺陷类型标签 */
    int class_id;                /* 类别 ID */
};

/* 单帧检测结果，包含所有检测框和性能指标 */
struct DetectionResult {
    std::vector<BoundingBox> detections;   /* 当前帧的所有检测框 */
    double inference_time_ms;              /* 推理耗时 (毫秒) */
    int image_width;                       /* 原图宽度 */
    int image_height;                      /* 原图高度 */
};

/* 相机后端类型，用于选择 OpenCV VideoCapture 后端 */
enum class CameraBackend {
    AutoDetect,  /* 自动探测最佳后端 */
    DShow,       /* DirectShow (Windows) */
    MSMF,        /* Media Foundation (Windows) */
    VFW,         /* Video for Windows */
    OpenCV       /* OpenCV 默认 */
};

/* 相机配置项 */
struct CameraConfig {
    int device_id = 0;            /* 相机设备号 */
    double exposure = -1;         /* 曝光时间 (μs)，-1 表示自动 */
    double gain = -1;             /* 增益 (dB)，-1 表示自动 */
    int width = 640;              /* 采集宽度 */
    int height = 480;             /* 采集高度 */
    double fps = 30;              /* 目标帧率 */
    CameraBackend backend = CameraBackend::DShow;  /* 视频后端 */
    int timeout_ms = 3000;        /* 帧读取超时 (毫秒) */
    int retry_count = 3;          /* 自动重连重试次数 */
    bool auto_reconnect = true;   /* 丢帧后自动重连 */
    int roi_x = 0, roi_y = 0, roi_w = 0, roi_h = 0;  /* ROI 区域，全 0 表示全图 */
};

/* UI 界面配置 */
struct UIConfig {
    int window_width = 1280;   /* 显示窗口宽度 */
    int window_height = 720;   /* 显示窗口高度 */
    bool show_fps = true;      /* 是否显示帧率 */
};

/* 串口通信配置 */
struct SerialConfig {
    std::string port = "COM1";  /* 串口号 */
    int baudrate = 9600;        /* 波特率 */
    int data_bits = 8;          /* 数据位 (5/6/7/8) */
    char parity = 'N';          /* 校验位: N=无, E=偶, O=奇 */
    int stop_bits = 1;          /* 停止位 (1/2) */
};

/* 全局应用配置，聚合所有模块的配置项 */
struct AppConfig {
    CameraConfig camera;                  /* 相机配置 */
    UIConfig ui;                          /* UI 配置 */
    SerialConfig serial;                  /* 串口配置 */
    std::string model_path = "models/model.onnx";  /* 模型路径 */
    float confidence_threshold = 0.5f;    /* 置信度阈值 */
    float nms_threshold = 0.45f;          /* NMS 的 IoU 阈值 */
    bool use_gpu = false;                 /* 是否启用 GPU 推理 */
    int gpu_device_id = 0;                /* GPU 设备号 */
};

/* 检测统计摘要，用于界面显示和报表导出 */
struct InspectionSummary {
    int total_inspected = 0;              /* 总检测数 */
    int pass_count = 0;                   /* 良品数 */
    int fail_count = 0;                   /* 缺陷数 */
    double yield_rate = 100.0;            /* 良率 (%) */
    std::vector<int> defect_counts;       /* 各类缺陷数量 */
    double avg_inference_time_ms = 0.0;   /* 平均推理耗时 */
};
