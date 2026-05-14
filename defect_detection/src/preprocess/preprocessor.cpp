#include "preprocess/preprocessor.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

Preprocessor::Preprocessor() = default;
Preprocessor::~Preprocessor() = default;

/* 图像缩放 */
cv::Mat Preprocessor::resize(const cv::Mat& src, int width, int height) {
    cv::Mat dst;
    cv::resize(src, dst, cv::Size(width, height));
    return dst;
}

/* 图像归一化：先除 255 缩放到 [0,1]，再按均值和标准差标准化 */
cv::Mat Preprocessor::normalize(const cv::Mat& src, const double* mean, const double* std) {
    cv::Mat dst;
    src.convertTo(dst, CV_32F);
    cv::divide(dst, 255.0, dst);
    if (mean && std) {
        cv::Mat mean_mat(src.size(), CV_32FC3, cv::Scalar(mean[0], mean[1], mean[2]));
        cv::Mat std_mat(src.size(), CV_32FC3, cv::Scalar(std[0], std[1], std[2]));
        dst = (dst - mean_mat) / std_mat;
    }
    return dst;
}

/* 直方图均衡化：增强对比度，对彩色图转灰度处理后还原 */
cv::Mat Preprocessor::equalize_hist(const cv::Mat& src) {
    cv::Mat dst;
    if (src.channels() == 1) {
        cv::equalizeHist(src, dst);
    } else {
        cv::Mat gray;
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        cv::cvtColor(gray, dst, cv::COLOR_GRAY2BGR);
    }
    return dst;
}

/* 非局部均值去噪 */
cv::Mat Preprocessor::denoise(const cv::Mat& src, double strength) {
    cv::Mat dst;
    cv::fastNlMeansDenoisingColored(src, dst, strength);
    return dst;
}

/* 仿射变换（用于图像校正、定位对齐） */
cv::Mat Preprocessor::warp_affine(const cv::Mat& src, const cv::Mat& M, cv::Size size) {
    cv::Mat dst;
    cv::warpAffine(src, dst, M, size);
    return dst;
}

/* 颜色空间转换，code 为 OpenCV 转换码如 cv::COLOR_BGR2GRAY */
cv::Mat Preprocessor::convert_color(const cv::Mat& src, int code) {
    cv::Mat dst;
    cv::cvtColor(src, dst, code);
    return dst;
}

/* 预处理流水线：输入多个预处理步骤的结果，目前为桩函数 */
std::vector<cv::Mat> Preprocessor::pipeline(const std::vector<cv::Mat>& stages) {
    return stages;
}
