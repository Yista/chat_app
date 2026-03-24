#ifndef FRIEND_HANDLER_HPP
#define FRIEND_HANDLER_HPP

#include <memory>
#include <string>
#include "database/friend_dao.hpp"
#include "database/user_dao.hpp"
#include "server/session_manager.hpp"

class FriendHandler {
public:
    FriendHandler(std::shared_ptr<FriendDAO> friendDao,
                  std::shared_ptr<UserDAO> userDao,
                  std::shared_ptr<SessionManager> sessionMgr);

    // HTTP 接口
    std::string handleSearchUsers(const std::string& body); // GET 参数从 query 解析，但为了统一，这里从body取JSON
    std::string handleSendRequest(const std::string& body);
    std::string handleRespondRequest(const std::string& body);
    std::string handleGetFriendList(const std::string& body);
    std::string handleGetRequests(const std::string& body);
    // WebSocket 通知（可选，但为了解耦，可以在 handler 内直接调用 sessionMgr 发送）
    void notifyFriendRequest(int targetUserId, const std::string& fromUsername);
    void notifyRequestResponse(int targetUserId, const std::string& message);

private:
    std::shared_ptr<FriendDAO> friendDao_;
    std::shared_ptr<UserDAO> userDao_;
    std::shared_ptr<SessionManager> sessionMgr_;
};

#endif