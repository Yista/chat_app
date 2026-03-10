#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <memory>
#include <string>
#include "utils/json.hpp"
#include "server/session.hpp"
#include "server/session_manager.hpp"
#include "database/user_dao.hpp"
#include "database/message_dao.hpp"
#include "database/friend_dao.hpp"

class MessageHandler {
public:
    MessageHandler(std::shared_ptr<SessionManager> sessionMgr,
                   std::shared_ptr<UserDAO> userDao,
                   std::shared_ptr<FriendDAO> friendDao,
                   std::shared_ptr<MessageDAO> messageDao);

    // 处理收到的 WebSocket 消息
    void handleMessage(std::shared_ptr<WebSocketSession> session, const std::string& message);
    std::string handleGetHistory(const std::string& body);

private:
    void handleLogin(std::shared_ptr<WebSocketSession> session, const nlohmann::json& j);
    void handlePrivate(std::shared_ptr<WebSocketSession> session, const nlohmann::json& j);
    void handlePublic(std::shared_ptr<WebSocketSession> session, const nlohmann::json& j);
    
    std::shared_ptr<SessionManager> sessionMgr_;
    std::shared_ptr<UserDAO> userDao_;
    std::shared_ptr<FriendDAO> friendDao_;
    std::shared_ptr<MessageDAO> messageDao_;
};

#endif