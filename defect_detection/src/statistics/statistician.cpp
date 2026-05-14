#include "statistics/statistician.hpp"
#include <fstream>
#include <numeric>
#include <iomanip>

Statistician::Statistician()
    : total_inspected_(0), pass_count_(0), fail_count_(0),
      defect_counts_(static_cast<int>(DefectType::Other) + 1, 0),
      total_inference_time_ms_(0.0) {}

Statistician::~Statistician() = default;

/* 记录一次完整的检测结果 */
void Statistician::record_detection(const DetectionResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    total_inspected_++;
    total_inference_time_ms_ += result.inference_time_ms;

    bool has_defect = false;
    for (const auto& box : result.detections) {
        if (box.class_id < static_cast<int>(defect_counts_.size())) {
            defect_counts_[box.class_id]++;
        }
        has_defect = true;
    }

    if (has_defect) {
        fail_count_++;
    } else {
        pass_count_++;
    }
}

/* 记录单个缺陷（无需完整 DetectionResult 时的快捷方式） */
void Statistician::record_defect(DefectType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    int idx = static_cast<int>(type);
    if (idx < static_cast<int>(defect_counts_.size())) {
        defect_counts_[idx]++;
    }
    fail_count_++;
    total_inspected_++;
}

/* 获取当前统计摘要 */
InspectionSummary Statistician::summary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    InspectionSummary s;
    s.total_inspected = total_inspected_;
    s.pass_count = pass_count_;
    s.fail_count = fail_count_;
    s.yield_rate = total_inspected_ > 0
        ? 100.0 * pass_count_ / total_inspected_
        : 100.0;
    s.defect_counts = defect_counts_;
    s.avg_inference_time_ms = total_inspected_ > 0
        ? total_inference_time_ms_ / total_inspected_
        : 0.0;
    return s;
}

/* 重置所有统计数据 */
void Statistician::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    total_inspected_ = 0;
    pass_count_ = 0;
    fail_count_ = 0;
    std::fill(defect_counts_.begin(), defect_counts_.end(), 0);
    total_inference_time_ms_ = 0.0;
}

/* 导出 CSV 格式统计报表 */
bool Statistician::export_csv(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "指标,值\n";
    file << "总检测数," << total_inspected_ << "\n";
    file << "良品数," << pass_count_ << "\n";
    file << "缺陷数," << fail_count_ << "\n";
    file << "良率," << std::fixed << std::setprecision(2) << yield_rate() << "\n";
    file << "平均推理耗时(ms)," << avg_cycle_time_ms() << "\n";

    for (int i = 0; i < static_cast<int>(defect_counts_.size()); ++i) {
        file << "缺陷类型" << i << "," << defect_counts_[i] << "\n";
    }

    return true;
}

/* 导出 JSON 格式统计报表 */
bool Statistician::export_json(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"total_inspected\": " << total_inspected_ << ",\n";
    file << "  \"pass_count\": " << pass_count_ << ",\n";
    file << "  \"fail_count\": " << fail_count_ << ",\n";
    file << "  \"yield_rate\": " << std::fixed << std::setprecision(2) << yield_rate() << ",\n";
    file << "  \"avg_inference_time_ms\": " << avg_cycle_time_ms() << ",\n";
    file << "  \"defect_counts\": [";
    for (size_t i = 0; i < defect_counts_.size(); ++i) {
        if (i > 0) file << ", ";
        file << defect_counts_[i];
    }
    file << "]\n}\n";
    return true;
}

double Statistician::yield_rate() const {
    return total_inspected_ > 0
        ? 100.0 * pass_count_ / total_inspected_
        : 100.0;
}

double Statistician::avg_cycle_time_ms() const {
    return total_inspected_ > 0
        ? total_inference_time_ms_ / total_inspected_
        : 0.0;
}

std::vector<int> Statistician::defect_distribution() const {
    return defect_counts_;
}
