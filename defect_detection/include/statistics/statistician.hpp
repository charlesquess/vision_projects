#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "utils/types.hpp"

/* 数据统计模块 - 记录检测结果，计算良率，导出报表 */
class Statistician {
public:
    Statistician();
    ~Statistician();

    void record_detection(const DetectionResult& result);  /* 记录一次检测结果 */
    void record_defect(DefectType type);                   /* 记录一个缺陷 */

    InspectionSummary summary() const;  /* 获取统计摘要 */
    void reset();                       /* 重置所有统计 */

    bool export_csv(const std::string& filepath) const;   /* 导出 CSV 报表 */
    bool export_json(const std::string& filepath) const;  /* 导出 JSON 报表 */

    double yield_rate() const;               /* 当前良率 */
    double avg_cycle_time_ms() const;        /* 平均检测周期 */
    std::vector<int> defect_distribution() const;  /* 缺陷分布 */

private:
    mutable std::mutex mutex_;       /* 线程安全互斥锁 */
    int total_inspected_;            /* 总检测数 */
    int pass_count_;                 /* 良品数 */
    int fail_count_;                 /* 缺陷数 */
    std::vector<int> defect_counts_; /* 各类缺陷计数 */
    double total_inference_time_ms_; /* 总推理耗时累计 */
};
