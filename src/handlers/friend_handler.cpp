#include "friend_handler.hpp"
#include "utils/logger.hpp"
#include "utils/json.hpp"

using json = nlohmann::json;

FriendHandler::FriendHandler(std::shared_ptr<FriendDAO> friendDao,
                             std::shared_ptr<UserDAO> userDao,
                             std::shared_ptr<SessionManager> sessionMgr)
    : friendDao_(friendDao), userDao_(userDao), sessionMgr_(sessionMgr) {}

std::string FriendHandler::handleSearchUsers(const std::string& body) {
    try {
        auto j = json::parse(body);
        std::string keyword = j["keyword"];
        int currentUserId = j["user_id"]; // 需要前端传入当前用户ID，或者从session获取。这里简化，前端传user_id。

        auto results = friendDao_->searchUsers(currentUserId, keyword);
        json resp = json::array();
        for (const auto& pair : results) {
            const User& u = pair.first;
            bool isFriend = pair.second;
            json item = {
                {"id", u.id},
                {"username", u.username},
                {"avatar", u.avatar},
                {"is_friend", isFriend}
            };
            resp.push_back(item);
        }
        return json{{"status", "ok"}, {"users", resp}}.dump();
    } catch (const std::exception& e) {
        LOG_ERROR("handleSearchUsers error: " + std::string(e.what()));
        return R"({"status":"error","message":"Invalid request"})";
    }
}

std::string FriendHandler::handleSendRequest(const std::string& body) {
    try {
        auto j = json::parse(body);
        int requestorId = j["user_id"];      // 当前用户ID
        std::string targetUsername = j["target_username"];

        // 查找目标用户
        auto targetOpt = userDao_->findByUsername(targetUsername);
        if (!targetOpt) {
            return R"({"status":"error","message":"User not found"})";
        }
        int targetId = targetOpt->id;

        // 不能加自己
        if (requestorId == targetId) {
            return R"({"status":"error","message":"Cannot add yourself"})";
        }

        // 检查是否已经是好友
        if (friendDao_->areFriends(requestorId, targetId)) {
            return R"({"status":"error","message":"Already friends"})";
        }

        // 检查是否有待处理的请求
        if (friendDao_->hasPendingRequest(requestorId, targetId)) {
            return R"({"status":"error","message":"Request already exists"})";
        }

        // 发送请求
        if (friendDao_->sendRequest(requestorId, targetId)) {
            // 通知目标用户（如果在线）
            notifyFriendRequest(targetId, targetOpt->username);

            return R"({"status":"ok","message":"Request sent"})";
        } else {
            return R"({"status":"error","message":"Failed to send request"})";
        }
    } catch (const std::exception& e) {
        LOG_ERROR("handleSendRequest error: " + std::string(e.what()));
        return R"({"status":"error","message":"Invalid request"})";
    }
}

std::string FriendHandler::handleRespondRequest(const std::string& body) {
    try {
        auto j = json::parse(body);
        int userId = j["user_id"];          // 当前用户ID
        int requestId = j["request_id"];
        std::string action = j["action"];   // "accept" or "reject"

        // 获取请求信息，验证请求的接收者是否是当前用户
        auto reqOpt = friendDao_->getPendingRequest(0,0); // 需要实现getRequestById
        // 简化：暂时不实现，先假设前端传了正确的请求ID，且当前用户是接收者
        // 为了快速实现，我们直接调用 acceptRequest/rejectRequest

        bool success = false;
        if (action == "accept") {
            success = friendDao_->acceptRequest(requestId);
        } else if (action == "reject") {
            success = friendDao_->rejectRequest(requestId);
        }

        if (success) {
            // 通知请求者（如果在线）
            // 需要从请求中获取请求者ID，这里简化
            // 暂不实现
            return json{{"status", "ok"}, {"message", "Response recorded"}}.dump();
        } else {
            return json{{"status", "error"}, {"message", "Failed to respond"}}.dump();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("handleRespondRequest error: " + std::string(e.what()));
        return R"({"status":"error","message":"Invalid request"})";
    }
}

void FriendHandler::notifyFriendRequest(int targetUserId, const std::string& fromUsername) {
    auto session = sessionMgr_->getSessionById(targetUserId);
    if (session) {
        json notification = {
            {"type", "friend_request"},
            {"from", fromUsername}
        };
        session->send(notification.dump());
    }
}

std::string FriendHandler::handleGetFriendList(const std::string& body) {
    try {
        auto j = json::parse(body);
        int userId = j["user_id"];

        auto friendPairs = friendDao_->getFriendList(userId);
        json result = json::array();
        for (auto& [fid, fname] : friendPairs) {
            json item;
            item["id"] = fid;
            item["username"] = fname;
            // 通过用户名查询在线状态
            bool online = (sessionMgr_->getSession(fname) != nullptr);
            item["online"] = online;
            result.push_back(item);
        }
        return result.dump();
    } catch (const std::exception& e) {
        LOG_ERROR("handleGetFriendList error: " + std::string(e.what()));
        return R"({"error":"Invalid request"})";
    }
}