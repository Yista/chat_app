#include "websocket_server.hpp"
#include "session.hpp"
#include "utils/logger.hpp"

WebSocketServer::WebSocketServer(net::io_context& ioc, unsigned short port)
    : ioc_(ioc), acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {}

void WebSocketServer::start() {
    doAccept();
    LOG_INFO("WebSocket server listening on port " + std::to_string(acceptor_.local_endpoint().port()));
}

void WebSocketServer::doAccept() {
    auto self = shared_from_this();
    acceptor_.async_accept(
        [self](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<WebSocketSession>(std::move(socket));
                if (self->newConnectionCallback_) {
                    self->newConnectionCallback_(session);
                }
                session->start();
            } else {
                LOG_ERROR("WebSocket accept error: " + ec.message());
            }
            self->doAccept();
        });
}