#ifndef FRIEND_DAO_HPP
#define FRIEND_DAO_HPP

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "database/db_manager.hpp"
#include "models/friendship.hpp"
#include "models/user.hpp"

class FriendDAO {
public:
    FriendDAO(std::shared_ptr<DBManager> db);

    // 初始化表（创建表）
    bool createTables();

    // 发送好友请求
    bool sendRequest(int requestorId, int requesteeId);

    // 检查是否存在待处理的请求（双向）
    bool hasPendingRequest(int userA, int userB);

    // 获取请求信息
    std::optional<FriendRequest> getRequest(int requestId);
    std::optional<FriendRequest> getPendingRequest(int requestorId, int requesteeId);

    // 接受请求
    bool acceptRequest(int requestId);

    // 拒绝请求
    bool rejectRequest(int requestId);

    // 获取用户的所有好友ID列表
    std::vector<int> getFriendIds(int userId);

    // 检查两个用户是否是好友
    bool areFriends(int userA, int userB);

    // 获取用户收到的待处理请求列表
    std::vector<FriendRequest> getIncomingRequests(int userId);

    // 获取用户发出的待处理请求列表
    std::vector<FriendRequest> getOutgoingRequests(int userId);

    // 搜索用户（根据用户名模糊匹配，排除自己，返回用户列表，附加是否是好友标记）
    std::vector<std::pair<User, bool>> searchUsers(int currentUserId, const std::string& keyword);

    // 获取好友列表，返回 (好友ID, 用户名) 的列表
    std::vector<std::pair<int, std::string>> getFriendList(int userId);
private:
    std::shared_ptr<DBManager> db_;
};

#endif