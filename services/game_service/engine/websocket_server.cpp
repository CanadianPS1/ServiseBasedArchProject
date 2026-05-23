#include <spdlog/spdlog.h>

#include "websocket_server.hpp"

namespace et_game {
WebsocketServer::WebsocketServer(std::uint16_t port) : _server(port) {}

void WebsocketServer::run() {
    spdlog::info("Starting WebSocket server on port {}...", _server.getPort());

    _server.setOnClientMessageCallback([this](ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg) {
        this->message_handler(conn, web_socket, msg);
    });

    auto listener = _server.listen();
    if(!listener.first) {
        spdlog::error("Failed to start WebSocket server: {}", listener.second);
        return;
    }

    _server.start();
    spdlog::info("WebSocket server started successfully on port {}!", _server.getPort());

    _server.wait();
}

void WebsocketServer::message_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg) {
    switch(msg->type) {
        case ix::WebSocketMessageType::Open:
            on_open_handler(conn, web_socket, msg);
            break;
        case ix::WebSocketMessageType::Close:
            on_close_handler(conn, web_socket, msg);
            break;
        case ix::WebSocketMessageType::Message:
            on_message_handler(conn, web_socket, msg);
            break;
        case ix::WebSocketMessageType::Error:
            on_error_handler(conn, web_socket, msg);
            break;
        default:
            spdlog::error("Received unknown message type from client {}: {}", conn->getId(), static_cast<int>(msg->type));
            break;
    }
}

void WebsocketServer::on_open_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg) {
    spdlog::info("Client connected: id={}, uri={}", conn->getId(), msg->openInfo.uri);
}

void WebsocketServer::on_close_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg) {
    spdlog::info("Client disconnected: id={}, code={}, reason='{}'",
        conn->getId(),
        msg->closeInfo.code,
        msg->closeInfo.reason
    );
}

void WebsocketServer::on_message_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg) {
    spdlog::info("Message received from client {}: {}", conn->getId(), msg->str);
}

void WebsocketServer::on_error_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg) {
    spdlog::error("Error from client {}: {}", conn->getId(), msg->errorInfo.reason);
}
} // namespace et_game
