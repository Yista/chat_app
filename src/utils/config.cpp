#include "config.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

class Config::Impl {
public:
    json data;
    bool loaded = false;
};

Config::Config(const std::string& filepath) : filepath_(filepath), pimpl_(new Impl) {}

Config::~Config() {
    delete pimpl_;
}

bool Config::load() {
    std::ifstream file(filepath_);
    if (!file.is_open()) {
        return false;
    }
    try {
        file >> pimpl_->data;
        pimpl_->loaded = true;
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<std::string> Config::getString(const std::string& key) const {
    if (!pimpl_->loaded) return std::nullopt;
    try {
        return pimpl_->data.at(key).get<std::string>();
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int> Config::getInt(const std::string& key) const {
    if (!pimpl_->loaded) return std::nullopt;
    try {
        return pimpl_->data.at(key).get<int>();
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> Config::getBool(const std::string& key) const {
    if (!pimpl_->loaded) return std::nullopt;
    try {
        return pimpl_->data.at(key).get<bool>();
    } catch (...) {
        return std::nullopt;
    }
}

// 快捷方法实现
std::string Config::mysqlHost() const {
    auto val = getString("mysql.host");
    return val.value_or("localhost");
}

int Config::mysqlPort() const {
    auto val = getInt("mysql.port");
    return val.value_or(3306);
}

std::string Config::mysqlUser() const {
    auto val = getString("mysql.user");
    return val.value_or("root");
}

std::string Config::mysqlPassword() const {
    auto val = getString("mysql.password");
    return val.value_or("");
}

std::string Config::mysqlDatabase() const {
    auto val = getString("mysql.database");
    return val.value_or("chatdb");
}

int Config::httpPort() const {
    auto val = getInt("http.port");
    return val.value_or(8080);
}

int Config::wsPort() const {
    auto val = getInt("websocket.port");
    return val.value_or(8081);
}