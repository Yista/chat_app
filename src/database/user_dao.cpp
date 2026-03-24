#include "user_dao.hpp"
#include "utils/logger.hpp"
#include <mysql/mysql.h>
#include <sstream>
#include <vector>

UserDAO::UserDAO(std::shared_ptr<DBManager> db) : db_(db) {}

bool UserDAO::createTable() {
    if (!db_) {
        LOG_ERROR("UserDAO::createTable: db_ is null");
        return false;
    }
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(50) UNIQUE NOT NULL,
            password_hash VARCHAR(255) NOT NULL,
            avatar VARCHAR(255),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            last_login TIMESTAMP NULL
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";
    return db_->execute(sql) >= 0;
}

int UserDAO::insert(const User& user) {
    if (!db_) {
        LOG_ERROR("UserDAO::insert: db_ is null");
        return 0;
    }
    MYSQL* conn = db_->getConnection();
    if (!conn) {
        LOG_ERROR("UserDAO::insert: Failed to get database connection");
        return 0;
    }

    // 实际应用中应使用预处理语句防止SQL注入，此处简化
    std::string sql = "INSERT INTO users (username, password_hash, avatar) VALUES ('"
                      + user.username + "', '" + user.password_hash + "', '"
                      + user.avatar + "')";
    if (db_->execute(sql) > 0) {
        return static_cast<int>(mysql_insert_id(conn));
    }
    return 0;
}

std::optional<User> UserDAO::findByUsername(const std::string& username) {
    if (!db_) {
        LOG_ERROR("UserDAO::findByUsername: db_ is null");
        return std::nullopt;
    }
    MYSQL* conn = db_->getConnection();
    if (!conn) {
        LOG_ERROR("UserDAO::findByUsername: Failed to get database connection");
        return std::nullopt;
    }

    std::string sql = "SELECT id, username, password_hash, avatar FROM users WHERE username = '"
                      + username + "'";
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("findByUsername query failed: " + std::string(mysql_error(conn)));
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        LOG_ERROR("mysql_store_result failed: " + std::string(mysql_error(conn)));
        return std::nullopt;
    }

    std::optional<User> result;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        User u;
        u.id = row[0] ? std::stoi(row[0]) : 0;
        u.username = row[1] ? row[1] : "";
        u.password_hash = row[2] ? row[2] : "";
        u.avatar = row[3] ? row[3] : "";
        result = u;
    }
    mysql_free_result(res);
    return result;
}

std::optional<User> UserDAO::findById(int id) {
    if (!db_) return std::nullopt;
    MYSQL* conn = db_->getConnection();
    if (!conn) return std::nullopt;
    // 类似实现，略
    return std::nullopt;
}

bool UserDAO::update(const User& user) {
    if (!db_) return false;
    MYSQL* conn = db_->getConnection();
    if (!conn) return false;
    // 暂未实现
    return false;
}

std::vector<std::pair<int, std::string>> UserDAO::searchUsers(const std::string& keyword, int excludeUserId) {
    std::vector<std::pair<int, std::string>> results;
    MYSQL* conn = db_->getConnection();
    if (!conn) return results;

    std::string sql;
    char escaped[256];
    if (keyword.empty()) {
        sql = "SELECT id, username FROM users WHERE id != " + std::to_string(excludeUserId) + " ORDER BY id";
    } else {
        mysql_real_escape_string(conn, escaped, keyword.c_str(), keyword.length());
        sql = "SELECT id, username FROM users WHERE id != " + std::to_string(excludeUserId)
            + " AND username LIKE '%" + escaped + "%' ORDER BY id";
    }

    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("searchUsers query failed: " + std::string(mysql_error(conn)));
        return results;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return results;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        int id = std::stoi(row[0]);
        std::string username = row[1] ? row[1] : "";
        results.emplace_back(id, username);
    }
    mysql_free_result(res);
    return results;
}