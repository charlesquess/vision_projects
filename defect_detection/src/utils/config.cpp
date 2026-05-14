#include "utils/config.hpp"
#include "utils/logger.hpp"

/* 配置管理器内部实现 (PIMPL) */
class Config::Impl {
public:
    /* 从 YAML 文件加载配置（桩函数） */
    bool load(const std::string& filepath) {
        /* TODO: 使用 yaml-cpp 解析 config.yaml */
        LOG_INFO("加载配置文件: " + filepath);
        loaded_ = true;
        return true;
    }

    /* 保存配置到文件（桩函数） */
    bool save(const std::string& filepath) const {
        /* TODO: 序列化到 YAML */
        (void)filepath;
        return true;
    }

    AppConfig app_config() const { return app_config_; }
    void set_app_config(const AppConfig& cfg) { app_config_ = cfg; }

    std::string dump() const { return "配置{...}"; }

private:
    AppConfig app_config_;    /* 应用配置结构体 */
    bool loaded_ = false;     /* 配置是否已加载 */
};

Config::Config() : impl_(std::make_unique<Impl>()) {}
Config::~Config() = default;
bool Config::load(const std::string& fp) { return impl_->load(fp); }
bool Config::save(const std::string& fp) const { return impl_->save(fp); }
AppConfig Config::app_config() const { return impl_->app_config(); }
void Config::set_app_config(const AppConfig& c) { impl_->set_app_config(c); }
std::string Config::dump() const { return impl_->dump(); }
