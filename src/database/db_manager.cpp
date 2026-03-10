#include "db_manager.hpp"
#include "utils/logger.hpp"
#include <mysql/mysql.h>
#include <cstring>

DBManager::DBManager(const std::string& host, int port,
                     const std::string& user, const std::string& password,
                     const std::string& database)
    : host_(host), port_(port), user_(user), password_(password), database_(database) {
    conn_ = mysql_init(nullptr);
    if (!conn_) {
        throw std::runtime_error("mysql_init failed");
    }
    connect();
}

DBManager::~DBManager() {
    if (conn_) {
        mysql_close(conn_);
    }
}

void DBManager::connect() {
    if (!mysql_real_connect(conn_, host_.c_str(), user_.c_str(),
                            password_.c_str(), database_.c_str(), port_, nullptr, 0)) {
        std::string err = "mysql_real_connect failed: ";
        err += mysql_error(conn_);
        throw std::runtime_error(err);
    }
    LOG_INFO("Connected to MySQL database: " + database_);
}

MYSQL* DBManager::getConnection() {
    // 检查连接是否有效，无效则重连
    if (!conn_ || mysql_ping(conn_) != 0) {
        if (conn_) mysql_close(conn_);
        conn_ = mysql_init(nullptr);
        if (!conn_) {
            LOG_ERROR("mysql_init failed during reconnect");
            return nullptr;
        }
        try {
            connect();  // 可能抛出异常
        } catch (const std::exception& e) {
            LOG_ERROR("Reconnect failed: " + std::string(e.what()));
            mysql_close(conn_);
            conn_ = nullptr;
            return nullptr;
        }
    }
    return conn_;
}

int DBManager::execute(const std::string& sql) {
    MYSQL* conn = getConnection();
    if (!conn) {
        LOG_ERROR("DBManager::execute: No database connection");
        return -1;
    }
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("MySQL query failed: " + std::string(mysql_error(conn)));
        return -1;
    }
    return static_cast<int>(mysql_affected_rows(conn));
}

bool DBManager::ping() {
    if (!conn_) return false;
    return mysql_ping(conn_) == 0;
}