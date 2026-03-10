#include "message_dao.hpp"
#include "utils/logger.hpp"
#include <mysql/mysql.h>
#include <sstream>
#include <iomanip>
#include <ctime>

MessageDAO::MessageDAO(std::shared_ptr<DBManager> db) : db_(db) {}

bool MessageDAO::createTables() {
    std::string sql1 = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INT AUTO_INCREMENT PRIMARY KEY,
            sender_id INT NOT NULL,
            receiver_id INT NOT NULL,
            content TEXT NOT NULL,
            msg_type TINYINT DEFAULT 0,
            status TINYINT DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (sender_id) REFERENCES users(id),
            FOREIGN KEY (receiver_id) REFERENCES users(id),
            INDEX idx_sender_receiver (sender_id, receiver_id),
            INDEX idx_created_at (created_at)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";

    std::string sql2 = R"(
        CREATE TABLE IF NOT EXISTS temp_message_counts (
            user_id1 INT NOT NULL,
            user_id2 INT NOT NULL,
            count INT DEFAULT 0,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY (user_id1, user_id2),
            FOREIGN KEY (user_id1) REFERENCES users(id),
            FOREIGN KEY (user_id2) REFERENCES users(id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )";

    bool ok1 = (db_->execute(sql1) >= 0);
    bool ok2 = (db_->execute(sql2) >= 0);
    if (ok1 && ok2) {
        LOG_INFO("Message tables created/verified");
    } else {
        LOG_ERROR("Failed to create message tables");
    }
    return ok1 && ok2;
}

int MessageDAO::insertMessage(int senderId, int receiverId, const std::string& content, int msgType) {
    MYSQL* conn = db_->getConnection();
    if (!conn) {
        LOG_ERROR("insertMessage: no database connection");
        return 0;
    }

    char escapedContent[content.length() * 2 + 1];
    mysql_real_escape_string(conn, escapedContent, content.c_str(), content.length());

    std::string sql = "INSERT INTO messages (sender_id, receiver_id, content, msg_type) VALUES ("
        + std::to_string(senderId) + ", " + std::to_string(receiverId) + ", '"
        + escapedContent + "', " + std::to_string(msgType) + ")";

    if (db_->execute(sql) > 0) {
        int id = static_cast<int>(mysql_insert_id(conn));
        LOG_DEBUG("Inserted message id: " + std::to_string(id));
        return id;
    }
    LOG_ERROR("Failed to insert message");
    return 0;
}

std::vector<Message> MessageDAO::getHistoryMessages(int userId1, int userId2, int limit) {
    std::vector<Message> messages;
    MYSQL* conn = db_->getConnection();
    if (!conn) return messages;

    // 注意：这里通过 JOIN 获取发送者的用户名
    std::string sql = "SELECT m.id, m.sender_id, m.receiver_id, u.username, m.content, m.msg_type, m.status, m.created_at "
                      "FROM messages m JOIN users u ON m.sender_id = u.id "
                      "WHERE (m.sender_id = " + std::to_string(userId1) + " AND m.receiver_id = " + std::to_string(userId2) + ") "
                      "OR (m.sender_id = " + std::to_string(userId2) + " AND m.receiver_id = " + std::to_string(userId1) + ") "
                      "ORDER BY m.created_at ASC LIMIT " + std::to_string(limit);

    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("getHistoryMessages query failed: " + std::string(mysql_error(conn)));
        return messages;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        LOG_ERROR("mysql_store_result failed: " + std::string(mysql_error(conn)));
        return messages;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        Message msg;
        msg.id = row[0] ? std::stoi(row[0]) : 0;
        msg.senderId = row[1] ? std::stoi(row[1]) : 0;
        msg.receiverId = row[2] ? std::stoi(row[2]) : 0;
        msg.senderName = row[3] ? row[3] : "";      // 从 JOIN 结果中获取
        msg.content = row[4] ? row[4] : "";
        msg.msgType = row[5] ? std::stoi(row[5]) : 0;
        msg.status = row[6] ? std::stoi(row[6]) : 0;
        msg.createdAt = row[7] ? row[7] : "";
        messages.push_back(msg);
    }
    mysql_free_result(res);
    LOG_DEBUG("Retrieved " + std::to_string(messages.size()) + " history messages");
    return messages;
}

bool MessageDAO::recallMessage(int messageId, int senderId) {
    MYSQL* conn = db_->getConnection();
    if (!conn) return false;

    std::string sql = "UPDATE messages SET status = 1 WHERE id = " + std::to_string(messageId)
                      + " AND sender_id = " + std::to_string(senderId) + " AND status = 0";
    int affected = db_->execute(sql);
    if (affected > 0) {
        LOG_INFO("Message recalled: id " + std::to_string(messageId));
        return true;
    }
    LOG_WARN("Recall failed: message not found or not allowed");
    return false;
}

bool MessageDAO::deleteMessage(int messageId, int userId) {
    MYSQL* conn = db_->getConnection();
    if (!conn) return false;

    std::string sql = "UPDATE messages SET status = 2 WHERE id = " + std::to_string(messageId)
                      + " AND (sender_id = " + std::to_string(userId) + " OR receiver_id = " + std::to_string(userId) + ")"
                      + " AND status = 0";
    int affected = db_->execute(sql);
    if (affected > 0) {
        LOG_INFO("Message deleted: id " + std::to_string(messageId));
        return true;
    }
    LOG_WARN("Delete failed: message not found or not allowed");
    return false;
}

int MessageDAO::getTempMessageCount(int userId1, int userId2) {
    int u1 = std::min(userId1, userId2);
    int u2 = std::max(userId1, userId2);
    MYSQL* conn = db_->getConnection();
    if (!conn) return 0;

    std::string sql = "SELECT count FROM temp_message_counts WHERE user_id1 = " + std::to_string(u1)
                      + " AND user_id2 = " + std::to_string(u2);
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("getTempMessageCount query failed: " + std::string(mysql_error(conn)));
        return 0;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return 0;
    int count = 0;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) {
        count = std::stoi(row[0]);
    }
    mysql_free_result(res);
    return count;
}

bool MessageDAO::incrementTempMessageCount(int userId1, int userId2) {
    int u1 = std::min(userId1, userId2);
    int u2 = std::max(userId1, userId2);
    MYSQL* conn = db_->getConnection();
    if (!conn) return false;

    std::string sql = "INSERT INTO temp_message_counts (user_id1, user_id2, count) VALUES ("
        + std::to_string(u1) + ", " + std::to_string(u2) + ", 1) "
        "ON DUPLICATE KEY UPDATE count = count + 1";
    int affected = db_->execute(sql);
    if (affected >= 0) {
        LOG_DEBUG("Incremented temp count for users " + std::to_string(u1) + " and " + std::to_string(u2));
        return true;
    }
    LOG_ERROR("Failed to increment temp message count");
    return false;
}

bool MessageDAO::resetTempMessageCount(int userId1, int userId2) {
    int u1 = std::min(userId1, userId2);
    int u2 = std::max(userId1, userId2);
    std::string sql = "DELETE FROM temp_message_counts WHERE user_id1 = " + std::to_string(u1)
                      + " AND user_id2 = " + std::to_string(u2);
    int affected = db_->execute(sql);
    if (affected >= 0) {
        LOG_DEBUG("Reset temp message count for users " + std::to_string(u1) + " and " + std::to_string(u2));
        return true;
    }
    return false;
}