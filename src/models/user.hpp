#ifndef USER_HPP
#define USER_HPP

#include <string>

struct User {
    int id = 0;
    std::string username;
    std::string password_hash;  // 实际存储哈希值，而非明文
    std::string avatar;         // 头像URL或路径
    // 可添加其他字段如 created_at, last_login 等，根据需要扩展
};

#endif