#include <iostream>
#include <csignal>
#ifdef _WIN32
#include <windows.h>
#endif
#include "utils/logger.hpp"
#include "utils/config.hpp"
#include "utils/timer.hpp"
#include "camera/camera.hpp"
#include "preprocess/preprocessor.hpp"
#include "inference/inferencer.hpp"
#include "postprocess/postprocessor.hpp"
#include "communication/serial_client.hpp"
#include "statistics/statistician.hpp"
#include "ui/ui_manager.hpp"

/* 全局运行标志，用于信号处理优雅退出 */
static volatile bool g_running = true;

void signal_handler(int) {
    g_running = false;
}

/* 程序入口：加载配置 → 初始化各模块 → 主循环 → 清理退出 */
int main(int argc, char* argv[]) {
    /* 设置控制台输出为 UTF-8，正确显示中文 */
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    /* 注册信号处理，支持 Ctrl+C 退出 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 初始化日志系统 */
    Logger::instance().set_level(LogLevel::Info);
    Logger::instance().set_log_file("logs/defect_detection.log");
    LOG_INFO("=== PCB 缺陷检测系统 v1.0.0 ===");

    /* ── 加载配置文件 ────────────────────────── */
    Config config;
    std::string config_path = "config/config.yaml";
    if (argc > 1) config_path = argv[1];
    if (!config.load(config_path)) {
        LOG_ERROR("加载配置文件失败: " + config_path);
        return 1;
    }
    AppConfig app_cfg = config.app_config();
    LOG_INFO("配置文件已加载: " + config_path);

    /* ── 初始化各模块 ────────────────────────── */
    Camera camera(app_cfg.camera);
    if (!camera.open()) {
        LOG_ERROR("相机初始化失败");
        return 1;
    }

    Preprocessor preprocessor;
    Inferencer inferencer;
    if (!inferencer.load_model(app_cfg.model_path, app_cfg.use_gpu, app_cfg.gpu_device_id)) {
        LOG_ERROR("模型加载失败: " + app_cfg.model_path);
        return 1;
    }
    LOG_INFO("模型已加载: " + app_cfg.model_path);

    Postprocessor postprocessor(app_cfg.confidence_threshold, app_cfg.nms_threshold);
    SerialClient serial(app_cfg.serial);
    Statistician statistician;
    UIManager ui;

    if (!ui.init_window("PCB Defect Detection", app_cfg.ui.window_width, app_cfg.ui.window_height)) {
        LOG_ERROR("UI 初始化失败");
        return 1;
    }

    serial.open();

    /* ── 主检测循环 ────────────────────────────────── */
    Timer frame_timer;
    int frame_count = 0;
    double fps = 0.0;

    LOG_INFO("系统就绪。按 ESC 或 Q 退出。");

    while (g_running) {
        frame_timer.start();

        /* 1. 采集一帧 */
        cv::Mat frame;
        if (!camera.capture_frame(frame)) {
            LOG_WARN("采集帧失败");
            break;
        }

        /* 2. 预处理：缩放到模型输入尺寸 */
        cv::Mat processed = preprocessor.resize(frame, 640, 640);

        /* 3. 推理 */
        auto results = inferencer.infer(processed);

        /* 4. 后处理 */
        std::vector<BoundingBox> detections;
        if (!results.empty()) {
            /* TODO: detections = postprocessor.process(...);
             *       detections = postprocessor.nms(detections); */
        }

        /* 5. 统计 */
        if (!results.empty()) {
            statistician.record_detection(results[0]);
        }

        /* 6. 显示 */
        cv::Mat display = frame.clone();
        postprocessor.draw_results(display, detections);
        auto summary = statistician.summary();
        ui.draw_overlay(display, summary);

        frame_count++;
        frame_timer.stop();
        double elapsed = frame_timer.elapsed_ms();
        fps = elapsed > 0 ? 1000.0 / elapsed : 0.0;
        ui.draw_info_panel(display, "FPS: " + std::to_string(static_cast<int>(fps)));

        ui.show_frame(display);

        /* 7. 通信：通过串口发送检测结果 */
        if (!detections.empty()) {
            std::string msg = "DEFECT:" + std::to_string(detections.size()) + "\n";
            serial.send(msg);
        } else {
            serial.send("PASS\n");
        }

        /* 8. 检测退出按键 */
        int key = ui.wait_key(1);
        if (key == 'q' || key == 'Q' || key == 27) break;
    }

    /* ── 清理退出 ──────────────────────────────────── */
    LOG_INFO("正在关闭系统...");
    statistician.export_csv("logs/stats_report.csv");

    serial.close();
    camera.close();
    ui.close_window();

    LOG_INFO("系统已安全关闭。");
    return 0;
}
