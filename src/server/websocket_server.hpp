#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include <boost/asio.hpp>
#include <memory>
#include <functional>

namespace net = boost::asio;
using tcp = net::ip::tcp;

// 前向声明
class WebSocketSession;

class WebSocketServer : public std::enable_shared_from_this<WebSocketServer> {
public:
    explicit WebSocketServer(net::io_context& ioc, unsigned short port);

    // 禁止拷贝
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    // 设置新连接回调
    void onNewConnection(std::function<void(std::shared_ptr<WebSocketSession>)> callback) {
        newConnectionCallback_ = std::move(callback);
    }

    void start();

private:
    void doAccept();

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::function<void(std::shared_ptr<WebSocketSession>)> newConnectionCallback_;
};

#endif