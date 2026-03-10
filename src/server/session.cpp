#include "session.hpp"
#include "utils/logger.hpp"
#include <boost/beast.hpp>

WebSocketSession::WebSocketSession(tcp::socket socket)
    : ws_(std::move(socket)) {}

WebSocketSession::~WebSocketSession() {
    LOG_DEBUG("WebSocketSession destroyed for " + remoteAddress());
}

void WebSocketSession::start() {
    auto self = shared_from_this();
    ws_.async_accept(
        [self](beast::error_code ec) {
            if (!ec) {
                LOG_INFO("WebSocket connection from " + self->remoteAddress());
                self->doRead();
            } else {
                LOG_ERROR("WebSocket handshake error: " + ec.message());
            }
        });
}

void WebSocketSession::send(const std::string& message) {
    bool shouldWrite = false;
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        shouldWrite = writeQueue_.empty();
        writeQueue_.push_back(message);
    }
    if (shouldWrite) {
        doWrite();
    }
}

void WebSocketSession::close() {
    auto self = shared_from_this();
    ws_.async_close(websocket::close_code::normal,
        [self](beast::error_code ec) {
            if (ec) {
                LOG_ERROR("WebSocket close error: " + ec.message());
            }
        });
}

void WebSocketSession::doRead() {
    auto self = shared_from_this();
    readBuffer_.clear();
    ws_.async_read(readBuffer_,
        [self](beast::error_code ec, std::size_t) {
            if (!ec) {
                std::string data = beast::buffers_to_string(self->readBuffer_.data());
                if (self->messageHandler_) {
                    self->messageHandler_(self, data);
                }
                self->doRead();
            } else if (ec == websocket::error::closed) {
                LOG_INFO("WebSocket closed normally: " + self->remoteAddress());
                if (self->closeHandler_) {
                    self->closeHandler_();
                }
            } else {
                LOG_ERROR("WebSocket read error: " + ec.message());
                if (self->closeHandler_) {
                    self->closeHandler_();
                }
            }
        });
}

void WebSocketSession::doWrite() {
    auto self = shared_from_this();
    std::string message;
    {
        std::lock_guard<std::mutex> lock(writeMutex_);
        if (writeQueue_.empty()) {
            return;
        }
        message = writeQueue_.front();
    }

    ws_.async_write(net::buffer(message),
        [self](beast::error_code ec, std::size_t) {
            if (!ec) {
                {
                    std::lock_guard<std::mutex> lock(self->writeMutex_);
                    if (!self->writeQueue_.empty()) {
                        self->writeQueue_.erase(self->writeQueue_.begin());
                    }
                }
                self->doWrite();
            } else {
                LOG_ERROR("WebSocket write error: " + ec.message());
            }
        });
}

std::string WebSocketSession::remoteAddress() const {
    try {
        auto ep = ws_.next_layer().socket().remote_endpoint();
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    } catch (...) {
        return "unknown";
    }
}