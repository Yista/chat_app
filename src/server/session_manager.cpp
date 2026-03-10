#include "session_manager.hpp"
#include "utils/logger.hpp"

void SessionManager::addSession(int userId, const std::string& username, std::shared_ptr<WebSocketSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessionsByUserId_[userId] = session;
    sessionsByUsername_[username] = userId;
    LOG_INFO("User online: " + username + " (userId: " + std::to_string(userId) + ", total: " + std::to_string(sessionsByUserId_.size()) + ")");
}

void SessionManager::removeSession(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessionsByUserId_.find(userId);
    if (it != sessionsByUserId_.end()) {
        // 获取用户名并从 username 映射中移除
        auto session = it->second;
        if (session) {
            std::string username = session->getUsername();
            sessionsByUsername_.erase(username);
        }
        sessionsByUserId_.erase(it);
        LOG_INFO("User offline: userId " + std::to_string(userId) + " (total: " + std::to_string(sessionsByUserId_.size()) + ")");
    }
}

std::shared_ptr<WebSocketSession> SessionManager::getSession(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessionsByUsername_.find(username);
    if (it != sessionsByUsername_.end()) {
        int userId = it->second;
        auto jt = sessionsByUserId_.find(userId);
        if (jt != sessionsByUserId_.end()) {
            return jt->second;
        }
    }
    return nullptr;
}

std::shared_ptr<WebSocketSession> SessionManager::getSessionById(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessionsByUserId_.find(userId);
    if (it != sessionsByUserId_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> SessionManager::getAllUsernames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> usernames;
    usernames.reserve(sessionsByUsername_.size());
    for (const auto& pair : sessionsByUsername_) {
        usernames.push_back(pair.first);
    }
    return usernames;
}