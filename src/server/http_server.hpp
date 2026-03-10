#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// HTTP 请求处理函数类型：接收请求体（字符串），返回响应体（JSON 字符串）
using HttpHandler = std::function<std::string(const std::string& body)>;

class HttpServer : public std::enable_shared_from_this<HttpServer> {
public:
    explicit HttpServer(net::io_context& ioc, unsigned short port);

    // 禁止拷贝
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // 注册路由
    void registerGet(const std::string& path, HttpHandler handler);
    void registerPost(const std::string& path, HttpHandler handler);

    // 启动服务器（开始监听）
    void start();

private:
    void doAccept();
    void handleRequest(std::shared_ptr<beast::tcp_stream> stream);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::unordered_map<std::string, HttpHandler> getHandlers_;
    std::unordered_map<std::string, HttpHandler> postHandlers_;
};

#endif