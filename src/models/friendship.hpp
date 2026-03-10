#ifndef FRIENDSHIP_HPP
#define FRIENDSHIP_HPP

#include <string>
#include <chrono>

struct FriendRequest {
    int id = 0;
    int requestorId = 0;
    int requesteeId = 0;
    std::string status; // "pending", "accepted", "rejected", "canceled"
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
};

struct Friendship {
    int userId1 = 0;
    int userId2 = 0;
    std::chrono::system_clock::time_point createdAt;
};

#endif