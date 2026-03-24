#include "utils/logger.hpp"
#include "utils/config.hpp"
#include "database/db_manager.hpp"
#include "database/user_dao.hpp"
#include "database/friend_dao.hpp"
#include "database/message_dao.hpp"
#include "server/http_server.hpp"
#include "server/websocket_server.hpp"
#include "server/session_manager.hpp"
#include "handlers/message_handler.hpp"
#include "handlers/auth_handler.hpp"
#include "handlers/friend_handler.hpp"
#include <memory>

int main() {
    Logger::setLevel(LOG_DEBUG);
    LOG_INFO("Chat server starting...");

    Config config("../config.json");
    if (!config.load()) {
        LOG_ERROR("Failed to load config.json");
        return 1;
    }

    // 初始化数据库
    std::shared_ptr<DBManager> db;
    std::shared_ptr<UserDAO> userDao;
    std::shared_ptr<FriendDAO> friendDao;
    std::shared_ptr<MessageDAO> messageDao;
    try {
        db = std::make_shared<DBManager>(
            config.mysqlHost(), config.mysqlPort(),
            config.mysqlUser(), config.mysqlPassword(),
            config.mysqlDatabase()
        );
        userDao = std::make_shared<UserDAO>(db);
        if (!userDao->createTable()) {
            LOG_ERROR("Failed to create users table");
            return 1;
        }
        LOG_INFO("Users table ready");

        friendDao = std::make_shared<FriendDAO>(db);
        if (!friendDao->createTables()) {
            LOG_ERROR("Failed to create friend tables");
            return 1;
        }
        LOG_INFO("Friend tables ready");

        messageDao = std::make_shared<MessageDAO>(db);
        if (!messageDao->createTables()) {
            LOG_ERROR("Failed to create message tables");
            return 1;
        }
        LOG_INFO("Message tables ready");
    } catch (const std::exception& e) {
        LOG_ERROR("Database error: " + std::string(e.what()));
        return 1;
    }

    // 创建会话管理器
    auto sessionMgr = std::make_shared<SessionManager>();

    // 创建消息处理器（WebSocket）
    auto msgHandler = std::make_shared<MessageHandler>(sessionMgr, userDao, friendDao, messageDao);

    // 创建认证处理器
    auto authHandler = std::make_shared<AuthHandler>(userDao);

    // 创建好友处理器
    auto friendHandler = std::make_shared<FriendHandler>(friendDao, userDao, sessionMgr);

    // 创建 io_context
    net::io_context ioc;

    // 创建 HTTP 服务器
    auto httpServer = std::make_shared<HttpServer>(ioc, config.httpPort());

    // 注册 HTTP 路由（注意捕获列表）
    httpServer->registerPost("/api/login", [authHandler](const std::string& body) {
        return authHandler->handleLogin(body);
    });

    httpServer->registerPost("/api/register", [authHandler](const std::string& body) {
        return authHandler->handleRegister(body);
    });

    httpServer->registerPost("/api/users/search", [friendHandler](const std::string& body) {
        return friendHandler->handleSearchUsers(body);
    });

    httpServer->registerPost("/api/friends/request", [friendHandler](const std::string& body) {
        return friendHandler->handleSendRequest(body);
    });

    httpServer->registerPost("/api/friends/requests", [friendHandler](const std::string& body) {
    return friendHandler->handleGetRequests(body);
    });

    httpServer->registerPost("/api/friends/respond", [friendHandler](const std::string& body) {
        return friendHandler->handleRespondRequest(body);
    });

    httpServer->registerPost("/api/friends/list", [friendHandler](const std::string& body) {
    return friendHandler->handleGetFriendList(body);
    });

    httpServer->registerPost("/api/messages/history", [msgHandler](const std::string& body) {
        return msgHandler->handleGetHistory(body);
    });

    httpServer->registerGet("/", [](const std::string&) {
        return R"({"message":"Chat server is running"})";
    });

    httpServer->start();

    // 创建 WebSocket 服务器
    auto wsServer = std::make_shared<WebSocketServer>(ioc, config.wsPort());

    wsServer->onNewConnection([sessionMgr, msgHandler](std::shared_ptr<WebSocketSession> session) {
        LOG_INFO("WebSocket client connected: " + session->remoteAddress());

        session->setMessageHandler([msgHandler](std::shared_ptr<WebSocketSession> sess, const std::string& msg) {
            msgHandler->handleMessage(sess, msg);
        });

        session->setCloseHandler([sessionMgr, session]() {
            if (session->isAuthenticated()) {
                sessionMgr->removeSession(session->getUserId());
            }
            LOG_INFO("WebSocket client disconnected: " + session->remoteAddress());
        });
    });

    wsServer->start();

    LOG_INFO("All servers started. Running io_context...");
    ioc.run();

    LOG_INFO("Server stopped.");
    return 0;
}