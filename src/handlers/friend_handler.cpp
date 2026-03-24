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
        int currentUserId = j["user_id"];

        // 从 UserDAO 获取用户列表
        auto users = userDao_->searchUsers(keyword, currentUserId);

        // 构造结果数组
        json result = json::array();
        for (const auto& [id, username] : users) {
            bool isFriend = friendDao_->areFriends(currentUserId, id);
            bool online = (sessionMgr_->getSession(username) != nullptr);
            result.push_back({
                {"id", id},
                {"username", username},
                {"is_friend", isFriend},
                {"online", online}
            });
        }

        // 排序：好友优先，然后按id升序
        std::sort(result.begin(), result.end(),
            [](const json& a, const json& b) {
                bool aFriend = a["is_friend"];
                bool bFriend = b["is_friend"];
                if (aFriend != bFriend) return aFriend > bFriend;
                return a["id"] < b["id"];
            });

        return result.dump();
    } catch (const std::exception& e) {
        LOG_ERROR("handleSearchUsers error: " + std::string(e.what()));
        return R"({"error":"Invalid request"})";
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
        int userId = j["user_id"];          // 当前用户ID（请求接收者）— 暂未使用，可保留
        int requestId = j["request_id"];
        std::string action = j["action"];   // "accept" or "reject"

        bool success = false;
        if (action == "accept") {
            success = friendDao_->acceptRequest(requestId);  // 修改为单参数
        } else if (action == "reject") {
            success = friendDao_->rejectRequest(requestId);  // 修改为单参数
        }

        if (success) {
            // 可选：通知请求者（需要从请求中获取请求者ID，这里暂不实现）
            return json{{"status", "ok"}, {"message", "操作成功"}}.dump();
        } else {
            return json{{"status", "error"}, {"message", "操作失败"}}.dump();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("handleRespondRequest error: " + std::string(e.what()));
        return R"({"status":"error","message":"Invalid request"})";
    }
}

// 获取待处理的好友请求列表
std::string FriendHandler::handleGetRequests(const std::string& body) {
    try {
        auto j = json::parse(body);
        int userId = j["user_id"];

        auto requests = friendDao_->getIncomingRequests(userId);
        json result = json::array();
        for (const auto& req : requests) {
            result.push_back({
                {"request_id", req.id},
                {"from_user_id", req.requestorId},
                {"from_username", req.requestorUsername},
                {"created_at", req.createdAt}
            });
        }
        return result.dump();
    } catch (const std::exception& e) {
        LOG_ERROR("handleGetRequests error: " + std::string(e.what()));
        return R"({"error":"Invalid request"})";
    }
}

// 通过 WebSocket 通知目标用户有新好友请求
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