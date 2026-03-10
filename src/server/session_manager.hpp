#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "server/session.hpp"

class SessionManager {
public:
    void addSession(int userId, const std::string& username, std::shared_ptr<WebSocketSession> session);
    void removeSession(int userId);
    std::shared_ptr<WebSocketSession> getSession(const std::string& username);
    std::shared_ptr<WebSocketSession> getSessionById(int userId);
    std::vector<std::string> getAllUsernames() const;

private:
    std::unordered_map<int, std::shared_ptr<WebSocketSession>> sessionsByUserId_;
    std::unordered_map<std::string, int> sessionsByUsername_;
    mutable std::mutex mutex_;
};

#endif