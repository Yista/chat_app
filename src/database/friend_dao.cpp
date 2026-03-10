#include "friend_dao.hpp"
#include "utils/logger.hpp"
#include <mysql/mysql.h>
#include <sstream>

FriendDAO::FriendDAO(std::shared_ptr<DBManager> db) : db_(db) {}

bool FriendDAO::createTables() {
    // 创建好友请求表
    const char* sqlRequests = R"(
        CREATE TABLE IF NOT EXISTS friend_requests (
            id INT AUTO_INCREMENT PRIMARY KEY,
            requestor_id INT NOT NULL,
            requestee_id INT NOT NULL,
            status ENUM('pending', 'accepted', 'rejected', 'canceled') DEFAULT 'pending',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            FOREIGN KEY (requestor_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY (requestee_id) REFERENCES users(id) ON DELETE CASCADE,
            UNIQUE KEY unique_request (requestor_id, requestee_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";

    // 创建好友关系表
    const char* sqlFriendships = R"(
        CREATE TABLE IF NOT EXISTS friendships (
            user_id1 INT NOT NULL,
            user_id2 INT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (user_id1, user_id2),
            FOREIGN KEY (user_id1) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY (user_id2) REFERENCES users(id) ON DELETE CASCADE,
            CHECK (user_id1 < user_id2)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";

    return db_->execute(sqlRequests) >= 0 && db_->execute(sqlFriendships) >= 0;
}

bool FriendDAO::sendRequest(int requestorId, int requesteeId) {
    // 不能对自己发送请求
    if (requestorId == requesteeId) return false;
    // 检查是否已经是好友
    if (areFriends(requestorId, requesteeId)) return false;
    // 检查是否存在待处理的请求（双向）
    if (hasPendingRequest(requestorId, requesteeId)) return false;

    std::string sql = "INSERT INTO friend_requests (requestor_id, requestee_id, status) VALUES ("
                      + std::to_string(requestorId) + ", " + std::to_string(requesteeId) + ", 'pending')";
    return db_->execute(sql) > 0;
}

bool FriendDAO::hasPendingRequest(int userA, int userB) {
    std::string sql = "SELECT id FROM friend_requests WHERE "
                      "((requestor_id = " + std::to_string(userA) + " AND requestee_id = " + std::to_string(userB) + ") "
                      "OR (requestor_id = " + std::to_string(userB) + " AND requestee_id = " + std::to_string(userA) + ")) "
                      "AND status = 'pending'";
    MYSQL* conn = db_->getConnection();
    if (!conn) return false;
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("hasPendingRequest query failed: " + std::string(mysql_error(conn)));
        return false;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return false;
    bool exists = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}

std::optional<FriendRequest> FriendDAO::getPendingRequest(int requestorId, int requesteeId) {
    std::string sql = "SELECT id, requestor_id, requestee_id, status, created_at, updated_at FROM friend_requests WHERE "
                      "requestor_id = " + std::to_string(requestorId) + " AND requestee_id = " + std::to_string(requesteeId) + " AND status = 'pending'";
    MYSQL* conn = db_->getConnection();
    if (!conn) return std::nullopt;
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("getPendingRequest query failed: " + std::string(mysql_error(conn)));
        return std::nullopt;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return std::nullopt;
    FriendRequest req;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        req.id = std::stoi(row[0]);
        req.requestorId = std::stoi(row[1]);
        req.requesteeId = std::stoi(row[2]);
        req.status = row[3];
        // 时间解析略
        mysql_free_result(res);
        return req;
    }
    mysql_free_result(res);
    return std::nullopt;
}

bool FriendDAO::acceptRequest(int requestId) {
    // 开始事务
    if (mysql_query(db_->getConnection(), "START TRANSACTION")) {
        LOG_ERROR("Failed to start transaction");
        return false;
    }

    // 获取请求信息
    std::string sql = "SELECT requestor_id, requestee_id FROM friend_requests WHERE id = " + std::to_string(requestId) + " AND status = 'pending'";
    MYSQL* conn = db_->getConnection();
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("acceptRequest query failed: " + std::string(mysql_error(conn)));
        mysql_query(conn, "ROLLBACK");
        return false;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        mysql_query(conn, "ROLLBACK");
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        mysql_query(conn, "ROLLBACK");
        return false;
    }
    int reqId = std::stoi(row[0]);
    int recId = std::stoi(row[1]);
    mysql_free_result(res);

    // 更新请求状态为 accepted
    std::string update = "UPDATE friend_requests SET status = 'accepted' WHERE id = " + std::to_string(requestId);
    if (db_->execute(update) <= 0) {
        mysql_query(conn, "ROLLBACK");
        return false;
    }

    // 插入好友关系表（确保 user_id1 < user_id2）
    int u1 = std::min(reqId, recId);
    int u2 = std::max(reqId, recId);
    std::string insert = "INSERT INTO friendships (user_id1, user_id2) VALUES (" + std::to_string(u1) + ", " + std::to_string(u2) + ")";
    if (db_->execute(insert) <= 0) {
        mysql_query(conn, "ROLLBACK");
        return false;
    }

    mysql_query(conn, "COMMIT");
    return true;
}

bool FriendDAO::rejectRequest(int requestId) {
    std::string sql = "UPDATE friend_requests SET status = 'rejected' WHERE id = " + std::to_string(requestId) + " AND status = 'pending'";
    return db_->execute(sql) > 0;
}

std::vector<int> FriendDAO::getFriendIds(int userId) {
    std::vector<int> friends;
    MYSQL* conn = db_->getConnection();
    if (!conn) return friends;

    // 查询 friendships 表中包含该用户的所有记录
    std::string sql = "SELECT user_id1, user_id2 FROM friendships WHERE user_id1 = " + std::to_string(userId) + " OR user_id2 = " + std::to_string(userId);
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("getFriendIds query failed: " + std::string(mysql_error(conn)));
        return friends;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return friends;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        int id1 = std::stoi(row[0]);
        int id2 = std::stoi(row[1]);
        friends.push_back(id1 == userId ? id2 : id1);
    }
    mysql_free_result(res);
    return friends;
}

bool FriendDAO::areFriends(int userA, int userB) {
    if (userA == userB) return false;
    int u1 = std::min(userA, userB);
    int u2 = std::max(userA, userB);
    std::string sql = "SELECT 1 FROM friendships WHERE user_id1 = " + std::to_string(u1) + " AND user_id2 = " + std::to_string(u2);
    MYSQL* conn = db_->getConnection();
    if (!conn) return false;
    if (mysql_query(conn, sql.c_str())) return false;
    MYSQL_RES* res = mysql_store_result(conn);
    bool exists = res && mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}

std::vector<FriendRequest> FriendDAO::getIncomingRequests(int userId) {
    std::vector<FriendRequest> requests;
    MYSQL* conn = db_->getConnection();
    if (!conn) return requests;
    std::string sql = "SELECT id, requestor_id, requestee_id, status, created_at FROM friend_requests WHERE requestee_id = " + std::to_string(userId) + " AND status = 'pending'";
    if (mysql_query(conn, sql.c_str())) return requests;
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return requests;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        FriendRequest req;
        req.id = std::stoi(row[0]);
        req.requestorId = std::stoi(row[1]);
        req.requesteeId = std::stoi(row[2]);
        req.status = row[3];
        // 时间略
        requests.push_back(req);
    }
    mysql_free_result(res);
    return requests;
}

std::vector<FriendRequest> FriendDAO::getOutgoingRequests(int userId) {
    std::vector<FriendRequest> requests;
    MYSQL* conn = db_->getConnection();
    if (!conn) return requests;
    std::string sql = "SELECT id, requestor_id, requestee_id, status, created_at FROM friend_requests WHERE requestor_id = " + std::to_string(userId) + " AND status = 'pending'";
    if (mysql_query(conn, sql.c_str())) return requests;
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return requests;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        FriendRequest req;
        req.id = std::stoi(row[0]);
        req.requestorId = std::stoi(row[1]);
        req.requesteeId = std::stoi(row[2]);
        req.status = row[3];
        requests.push_back(req);
    }
    mysql_free_result(res);
    return requests;
}

std::vector<std::pair<User, bool>> FriendDAO::searchUsers(int currentUserId, const std::string& keyword) {
    std::vector<std::pair<User, bool>> results;
    MYSQL* conn = db_->getConnection();
    if (!conn) return results;

    // 模糊搜索用户名，排除当前用户
    std::string sql = "SELECT id, username, password_hash, avatar FROM users WHERE id != " + std::to_string(currentUserId) + " AND username LIKE '%" + keyword + "%'";
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("searchUsers query failed: " + std::string(mysql_error(conn)));
        return results;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return results;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        User u;
        u.id = std::stoi(row[0]);
        u.username = row[1];
        u.password_hash = row[2];
        u.avatar = row[3] ? row[3] : "";
        bool isFriend = areFriends(currentUserId, u.id);
        results.emplace_back(u, isFriend);
    }
    mysql_free_result(res);
    return results;
}

std::vector<std::pair<int, std::string>> FriendDAO::getFriendList(int userId) {
    std::vector<std::pair<int, std::string>> result;
    MYSQL* conn = db_->getConnection();
    if (!conn) return result;

    // 查询好友ID及用户名（通过联合查询）
    std::string sql = "SELECT u.id, u.username FROM users u WHERE u.id IN ("
                      "SELECT user_id2 FROM friendships WHERE user_id1 = " + std::to_string(userId)
                      + " UNION SELECT user_id1 FROM friendships WHERE user_id2 = " + std::to_string(userId) + ")";
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("getFriendList query failed: " + std::string(mysql_error(conn)));
        return result;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return result;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        int id = std::stoi(row[0]);
        std::string username = row[1] ? row[1] : "";
        result.emplace_back(id, username);
    }
    mysql_free_result(res);
    return result;
}