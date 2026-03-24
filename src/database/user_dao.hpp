#ifndef USER_DAO_HPP
#define USER_DAO_HPP

#include <memory>
#include <string>
#include <optional>
#include "database/db_manager.hpp"
#include "models/user.hpp"
#include <vector>

class UserDAO {
public:
    UserDAO(std::shared_ptr<DBManager> db);

    // 创建用户表（若不存在）
    bool createTable();

    // 插入新用户，返回自增ID（0表示失败）
    int insert(const User& user);

    // 根据用户名查找用户
    std::optional<User> findByUsername(const std::string& username);

    // 根据用户ID查找用户
    std::optional<User> findById(int id);

    // 查找所有用户（按关键字查找，默认查找所有用户）
    std::vector<std::pair<int, std::string>> searchUsers(const std::string& keyword, int excludeUserId);

    // 更新用户信息（如最后登录时间）
    bool update(const User& user);

private:
    std::shared_ptr<DBManager> db_;
};

#endif