#include "http_server.hpp"
#include "utils/logger.hpp"
#include <boost/beast/version.hpp>

HttpServer::HttpServer(net::io_context& ioc, unsigned short port)
    : ioc_(ioc), acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {}

void HttpServer::registerGet(const std::string& path, HttpHandler handler) {
    getHandlers_[path] = std::move(handler);
}

void HttpServer::registerPost(const std::string& path, HttpHandler handler) {
    postHandlers_[path] = std::move(handler);
}

void HttpServer::start() {
    doAccept();
    LOG_INFO("HTTP server listening on port " + std::to_string(acceptor_.local_endpoint().port()));
}

void HttpServer::doAccept() {
    auto self = shared_from_this();
    acceptor_.async_accept(
        [self](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto stream = std::make_shared<beast::tcp_stream>(std::move(socket));
                self->handleRequest(stream);
            } else {
                LOG_ERROR("HTTP accept error: " + ec.message());
            }
            self->doAccept();
        });
}

void HttpServer::handleRequest(std::shared_ptr<beast::tcp_stream> stream) {
    auto self = shared_from_this();
    auto buffer = std::make_shared<beast::flat_buffer>();
    auto req = std::make_shared<http::request<http::string_body>>();

    http::async_read(*stream, *buffer, *req,
        [self, stream, buffer, req](beast::error_code ec, std::size_t) {
            if (ec == http::error::end_of_stream) {
                stream->socket().shutdown(tcp::socket::shutdown_send, ec);
                return;
            }
            if (ec) {
                LOG_ERROR("HTTP read error: " + ec.message());
                return;
            }

            // 基础响应框架
            http::response<http::string_body> res;
            res.version(req->version());
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "application/json");

            // CORS 头（所有响应都添加）
            res.set(http::field::access_control_allow_origin, "*");
            res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
            res.set(http::field::access_control_allow_headers, "Content-Type");

            std::string path(req->target());

            // 处理 OPTIONS 预检请求
            if (req->method() == http::verb::options) {
                res.result(http::status::no_content); // 204 No Content
                res.body() = "";
                res.prepare_payload();
                auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
                http::async_write(*stream, *sp,
                    [stream, sp](beast::error_code ec, std::size_t) {
                        if (ec) LOG_ERROR("HTTP write error: " + ec.message());
                        stream->socket().shutdown(tcp::socket::shutdown_send, ec);
                    });
                return;
            }

            // 处理 GET/POST 请求
            HttpHandler handler;
            if (req->method() == http::verb::get) {
                auto it = self->getHandlers_.find(path);
                if (it != self->getHandlers_.end()) {
                    handler = it->second;
                }
            } else if (req->method() == http::verb::post) {
                auto it = self->postHandlers_.find(path);
                if (it != self->postHandlers_.end()) {
                    handler = it->second;
                }
            }

            if (handler) {
                std::string response_body = handler(req->body());
                res.result(http::status::ok);
                res.body() = std::move(response_body);
                res.prepare_payload();
            } else {
                res.result(http::status::not_found);
                res.body() = R"({"error":"Not found"})";
                res.prepare_payload();
            }

            auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
            http::async_write(*stream, *sp,
                [stream, sp](beast::error_code ec, std::size_t) {
                    if (ec) LOG_ERROR("HTTP write error: " + ec.message());
                    stream->socket().shutdown(tcp::socket::shutdown_send, ec);
                });
        });
}