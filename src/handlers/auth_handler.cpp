#include "auth_handler.hpp"
#include "utils/logger.hpp"
#include "utils/json.hpp"

using json = nlohmann::json;

AuthHandler::AuthHandler(std::shared_ptr<UserDAO> userDao) : userDao_(userDao) {}

std::string AuthHandler::handleLogin(const std::string& body) {
    if (!userDao_) {
        LOG_ERROR("AuthHandler::handleLogin: userDao_ is null");
        return R"({"status":"error","message":"Internal server error"})";
    }
    try {
        auto j = json::parse(body);
        std::string username = j["username"];
        std::string password = j["password"]; // 实际应使用哈希

        auto user = userDao_->findByUsername(username);
        if (user && user->password_hash == password) {
            json resp = {{"status","ok"}, {"message","Login successful"}, {"user_id", user->id}};
            return resp.dump();
        } else {
            return R"({"status":"error","message":"Invalid credentials"})";
        }
    } catch (const std::exception& e) {
        LOG_ERROR("AuthHandler::handleLogin exception: " + std::string(e.what()));
        return R"({"status":"error","message":"Invalid JSON"})";
    }
}

std::string AuthHandler::handleRegister(const std::string& body) {
    if (!userDao_) {
        LOG_ERROR("AuthHandler::handleRegister: userDao_ is null");
        return R"({"status":"error","message":"Internal server error"})";
    }
    try {
        auto j = json::parse(body);
        User user;
        user.username = j["username"];
        user.password_hash = j["password"]; // 实际应哈希
        user.avatar = j.value("avatar", "");

        auto existing = userDao_->findByUsername(user.username);
        if (existing) {
            return R"({"status":"error","message":"Username already exists"})";
        }

        int id = userDao_->insert(user);
        if (id > 0) {
            json resp = {{"status","ok"}, {"message","Register successful"}, {"user_id", id}};
            return resp.dump();
        } else {
            return R"({"status":"error","message":"Register failed"})";
        }
    } catch (const std::exception& e) {
        LOG_ERROR("AuthHandler::handleRegister exception: " + std::string(e.what()));
        return R"({"status":"error","message":"Invalid JSON"})";
    }
}