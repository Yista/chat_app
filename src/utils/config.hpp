#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <optional>

class Config {
public:
    Config(const std::string& filepath);
    ~Config();

    // 加载配置文件，成功返回true
    bool load();

    // 获取配置值，若不存在返回空optional
    std::optional<std::string> getString(const std::string& key) const;
    std::optional<int> getInt(const std::string& key) const;
    std::optional<bool> getBool(const std::string& key) const;

    // 常用快捷方法（根据你的config.json结构定义）
    std::string mysqlHost() const;
    int mysqlPort() const;
    std::string mysqlUser() const;
    std::string mysqlPassword() const;
    std::string mysqlDatabase() const;
    int httpPort() const;
    int wsPort() const;

private:
    std::string filepath_;
    class Impl;
    Impl* pimpl_;
};

#endif