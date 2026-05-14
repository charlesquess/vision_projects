#include "communication/serial_client.hpp"
#include "utils/logger.hpp"

/* 内部实现类 (PIMPL 模式) */
class SerialClient::Impl {
public:
    explicit Impl(const SerialConfig& cfg) : config_(cfg) {}

    /* 打开串口（桩函数） */
    bool open() {
        LOG_INFO("正在打开串口: " + config_.port);
        /* TODO: 平台相关实现
         * Windows: CreateFile("\\\\.\\COM1", ...)
         * Linux:   open("/dev/ttyS0", ...) */
        is_open_ = true;
        return true;
    }

    void close() {
        is_open_ = false;
    }

    bool is_open() const { return is_open_; }

    /* 发送二进制数据（桩函数） */
    bool send(const std::vector<uint8_t>& data) {
        if (!is_open_) return false;
        /* TODO: 实现串口写入 */
        (void)data;
        return true;
    }

    /* 发送文本消息 */
    bool send(const std::string& message) {
        std::vector<uint8_t> data(message.begin(), message.end());
        return send(data);
    }

    /* 接收数据，带超时（桩函数） */
    std::vector<uint8_t> receive(size_t timeout_ms) {
        /* TODO: 实现串口读取 */
        (void)timeout_ms;
        return {};
    }

    /* 设置波特率 */
    bool set_baudrate(int baudrate) {
        config_.baudrate = baudrate;
        /* TODO: 应用波特率更改 */
        return true;
    }

    bool set_parity(char parity) {
        config_.parity = parity;
        return true;
    }

    bool set_data_bits(int bits) {
        config_.data_bits = bits;
        return true;
    }

    bool set_stop_bits(int bits) {
        config_.stop_bits = bits;
        return true;
    }

    void set_receive_callback(DataCallback cb) {
        callback_ = cb;
    }

    SerialConfig config() const { return config_; }

private:
    SerialConfig config_;          /* 串口配置 */
    bool is_open_ = false;         /* 串口是否已打开 */
    DataCallback callback_;        /* 异步接收回调 */
};

SerialClient::SerialClient(const SerialConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}
SerialClient::~SerialClient() = default;
bool SerialClient::open() { return impl_->open(); }
void SerialClient::close() { impl_->close(); }
bool SerialClient::is_open() const { return impl_->is_open(); }
bool SerialClient::send(const std::vector<uint8_t>& d) { return impl_->send(d); }
bool SerialClient::send(const std::string& m) { return impl_->send(m); }
std::vector<uint8_t> SerialClient::receive(size_t t) { return impl_->receive(t); }
bool SerialClient::set_baudrate(int b) { return impl_->set_baudrate(b); }
bool SerialClient::set_parity(char p) { return impl_->set_parity(p); }
bool SerialClient::set_data_bits(int b) { return impl_->set_data_bits(b); }
bool SerialClient::set_stop_bits(int b) { return impl_->set_stop_bits(b); }
void SerialClient::set_receive_callback(DataCallback cb) { impl_->set_receive_callback(cb); }
SerialConfig SerialClient::config() const { return impl_->config(); }
