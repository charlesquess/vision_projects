#include "inference/inferencer.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <cstring>

/* 内部实现类 (PIMPL 模式) */
class Inferencer::Impl {
public:
    Impl() = default;

    /* 加载 ONNX 模型，支持 CPU/GPU 推理 */
    bool load_model(const std::string& model_path, bool use_gpu, int gpu_id) {
#ifdef USE_ONNX_RUNTIME
        /* 实际实现：创建 Ort::Session，配置 SessionOptions */
        LOG_INFO("正在加载 ONNX 模型: " + model_path);
        model_path_ = model_path;
        loaded_ = true;
        return true;
#else
        LOG_WARN("ONNX Runtime 未启用，模型加载为桩函数。");
        model_path_ = model_path;
        loaded_ = true;
        return true;
#endif
    }

    void unload_model() {
        loaded_ = false;
    }

    /* 对单张图像进行推理（桩实现，返回空结果） */
    std::vector<DetectionResult> infer(const cv::Mat& image) {
        Timer t("推理");
        DetectionResult result;
        result.image_width = image.cols;
        result.image_height = image.rows;

        /* 构建模型输入的 blob */
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0 / 255.0, cv::Size(640, 640),
                               cv::Scalar(), true, false);

        /* TODO: 运行 ONNX 会话，解析输出张量 */
        result.inference_time_ms = t.elapsed_ms();

        std::vector<DetectionResult> batch;
        batch.push_back(result);
        return batch;
    }

    /* 批量推理 */
    std::vector<DetectionResult> infer_batch(const std::vector<cv::Mat>& images) {
        std::vector<DetectionResult> results;
        for (const auto& img : images) {
            auto res = infer(img);
            results.insert(results.end(), res.begin(), res.end());
        }
        return results;
    }

    ModelInfo model_info() const { return info_; }
    bool is_loaded() const { return loaded_; }

private:
    std::string model_path_;      /* 模型文件路径 */
    ModelInfo info_;               /* 模型张量信息 */
    bool loaded_ = false;          /* 模型是否已加载 */
};

Inferencer::Inferencer() : impl_(std::make_unique<Impl>()) {}
Inferencer::~Inferencer() = default;
bool Inferencer::load_model(const std::string& path, bool gpu, int id) { return impl_->load_model(path, gpu, id); }
void Inferencer::unload_model() { impl_->unload_model(); }
std::vector<DetectionResult> Inferencer::infer(const cv::Mat& img) { return impl_->infer(img); }
std::vector<DetectionResult> Inferencer::infer_batch(const std::vector<cv::Mat>& imgs) { return impl_->infer_batch(imgs); }
ModelInfo Inferencer::model_info() const { return impl_->model_info(); }
bool Inferencer::is_loaded() const { return impl_->is_loaded(); }
