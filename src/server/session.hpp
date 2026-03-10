#ifndef SESSION_HPP
#define SESSION_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <vector>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    explicit WebSocketSession(tcp::socket socket);
    ~WebSocketSession();

    // 禁止拷贝
    WebSocketSession(const WebSocketSession&) = delete;
    WebSocketSession& operator=(const WebSocketSession&) = delete;

    // 启动会话（握手）
    void start();

    // 发送消息（线程安全）
    void send(const std::string& message);

    // 主动关闭连接
    void close();

    // 设置消息回调
    void setMessageHandler(std::function<void(std::shared_ptr<WebSocketSession>, const std::string&)> handler) {
        messageHandler_ = std::move(handler);
    }

    // 设置关闭回调
    void setCloseHandler(std::function<void()> handler) {
        closeHandler_ = std::move(handler);
    }

    // 获取远程地址
    std::string remoteAddress() const;

    // 用户状态管理
    void setUsername(const std::string& name) { username_ = name; }
    std::string getUsername() const { return username_; }
    void setAuthenticated(bool auth) { authenticated_ = auth; }
    bool isAuthenticated() const { return authenticated_; }
    void setUserId(int id) { userId_ = id; }
    int getUserId() const { return userId_; }

private:
    void doRead();
    void doWrite();
    void onWrite(beast::error_code ec, std::size_t bytes_transferred);

    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer readBuffer_;
    std::vector<std::string> writeQueue_;
    std::mutex writeMutex_;
    std::function<void(std::shared_ptr<WebSocketSession>, const std::string&)> messageHandler_;
    std::function<void()> closeHandler_;

    // 用户信息
    std::string username_;
    bool authenticated_ = false;
    int userId_ = 0;
};

#endif