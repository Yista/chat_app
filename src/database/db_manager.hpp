#ifndef DB_MANAGER_HPP
#define DB_MANAGER_HPP

#include <mysql/mysql.h>
#include <string>
#include <memory>
#include <stdexcept>

class DBManager {
public:
    DBManager(const std::string& host, int port,
              const std::string& user, const std::string& password,
              const std::string& database);
    ~DBManager();

    // 禁止拷贝
    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    // 获取原生连接句柄（供 DAO 使用）
    MYSQL* getConnection();

    // 执行简单查询（无结果集），返回 affected rows
    int execute(const std::string& sql);

    // 检查连接是否正常
    bool ping();

private:
    MYSQL* conn_;
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;

    void connect();
};

#endif