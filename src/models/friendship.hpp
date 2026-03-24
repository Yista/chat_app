#ifndef FRIENDSHIP_HPP
#define FRIENDSHIP_HPP

#include <string>
#include <chrono>

struct FriendRequest {
    int id = 0;
    int requestorId = 0;
    int requesteeId = 0;
    std::string status; // "pending", "accepted", "rejected", "canceled"
    std::string createdAt;           // 改为字符串，存储从数据库读出的时间
    std::string requestorUsername;   // 新增，存储请求者用户名
};

struct Friendship {
    int userId1 = 0;
    int userId2 = 0;
    std::chrono::system_clock::time_point createdAt;
};

#endif