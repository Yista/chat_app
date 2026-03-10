#include "message_handler.hpp"
#include "utils/logger.hpp"
#include "utils/json.hpp"

using json = nlohmann::json;

MessageHandler::MessageHandler(std::shared_ptr<SessionManager> sessionMgr,
                               std::shared_ptr<UserDAO> userDao,
                               std::shared_ptr<FriendDAO> friendDao,
                               std::shared_ptr<MessageDAO> messageDao)
    : sessionMgr_(sessionMgr), userDao_(userDao), friendDao_(friendDao), messageDao_(messageDao) {}

void MessageHandler::handleMessage(std::shared_ptr<WebSocketSession> session, const std::string& message) {
    try {
        auto j = json::parse(message);
        std::string type = j["type"];

        if (type == "login") {
            handleLogin(session, j);
        } else if (type == "private") {
            handlePrivate(session, j);
        } else if (type == "public") {
            handlePublic(session, j);
        } else {
            session->send(R"({"type":"error","message":"Unknown message type"})");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parse error: " + std::string(e.what()));
        session->send(R"({"type":"error","message":"Invalid JSON"})");
    }
}

void MessageHandler::handleLogin(std::shared_ptr<WebSocketSession> session, const json& j) {
    std::string username = j["username"];
    std::string password = j["password"]; // 实际应使用哈希

    auto user = userDao_->findByUsername(username);
    if (user && user->password_hash == password) {
        // 如果该用户已在线，先踢下线
        auto existing = sessionMgr_->getSession(username);
        if (existing) {
            existing->send(R"({"type":"system","message":"You have been logged in elsewhere"})");
            existing->close();
            // 从管理器移除旧会话（使用旧用户的ID）
            sessionMgr_->removeSession(existing->getUserId());
        }

        session->setUsername(username);
        session->setAuthenticated(true);
        session->setUserId(user->id);

        sessionMgr_->addSession(user->id, username, session);
        session->send(R"({"type":"login_result","success":true})");
        LOG_INFO("User logged in: " + username);
    } else {
        session->send(R"({"type":"login_result","success":false,"message":"Invalid credentials"})");
    }
}

void MessageHandler::handlePrivate(std::shared_ptr<WebSocketSession> session, const json& j) {
    if (!session->isAuthenticated()) {
        session->send(R"({"type":"error","message":"Not logged in"})");
        return;
    }

    std::string toUsername = j["to"];
    std::string content = j["content"];
    int senderId = session->getUserId();

    // 查找目标用户
    auto targetOpt = userDao_->findByUsername(toUsername);
    if (!targetOpt) {
        session->send(R"({"type":"error","message":"User not found"})");
        return;
    }
    int targetId = targetOpt->id;

    // 检查是否为好友
    bool areFriends = friendDao_->areFriends(senderId, targetId);
    if (!areFriends) {
        // 检查临时消息计数
        int count = messageDao_->getTempMessageCount(senderId, targetId);
        if (count >= 5) {
            session->send(R"({"type":"error","message":"You can only send 5 messages to non-friends"})");
            return;
        }
        // 增加计数
        messageDao_->incrementTempMessageCount(senderId, targetId);
    }

    // 保存消息到数据库
    int msgId = messageDao_->insertMessage(senderId, targetId, content, 0);
    if (msgId == 0) {
        session->send(R"({"type":"error","message":"Failed to save message"})");
        return;
    }

    // 构造转发消息
    json outgoing;
    outgoing["type"] = "private";
    outgoing["from"] = session->getUsername();
    outgoing["from_id"] = senderId;
    outgoing["content"] = content;
    outgoing["msg_id"] = msgId;
    outgoing["created_at"] = "TODO"; // 可以从数据库获取，但简单起见由客户端生成时间

    // 如果目标在线，发送
    auto targetSession = sessionMgr_->getSession(toUsername);
    if (targetSession) {
        targetSession->send(outgoing.dump());
    } else {
        // 离线消息，可存储待拉取（后续实现）
    }

    // 发送回执给发送者（可选）
    session->send(R"({"type":"private_sent","to":")" + toUsername + R"(","msg_id":)" + std::to_string(msgId) + "}");
}

void MessageHandler::handlePublic(std::shared_ptr<WebSocketSession> session, const json& j) {
    if (!session->isAuthenticated()) {
        session->send(R"({"type":"error","message":"Not logged in"})");
        return;
    }

    std::string content = j["content"];
    std::string from = session->getUsername();

    json response = {
        {"type", "public"},
        {"from", from},
        {"content", content}
    };
    std::string responseStr = response.dump();

    auto allUsers = sessionMgr_->getAllUsernames();
    for (const auto& username : allUsers) {
        if (username != from) {
            auto s = sessionMgr_->getSession(username);
            if (s) s->send(responseStr);
        }
    }
}

std::string MessageHandler::handleGetHistory(const std::string& body) {
    try {
        auto j = json::parse(body);
        int userId = j["user_id"];
        int otherId = j["other_id"];
        int limit = j.value("limit", 50);

        auto msgs = messageDao_->getHistoryMessages(userId, otherId, limit);
        json result = json::array();
        for (auto& msg : msgs) {
            json item;
            item["id"] = msg.id;
            item["sender_id"] = msg.senderId;
            item["receiver_id"] = msg.receiverId;
            item["sender_username"] = msg.senderName;
            item["content"] = msg.content;
            item["msg_type"] = msg.msgType;
            item["status"] = msg.status;
            item["created_at"] = msg.createdAt;
            result.push_back(item);
        }
        return result.dump();
    } catch (const std::exception& e) {
        LOG_ERROR("handleGetHistory error: " + std::string(e.what()));
        return R"({"error":"Invalid request"})";
    }
}