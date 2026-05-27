#include <unordered_map>

#include <spdlog/spdlog.h>

#include "engine/network/websocket_server.hpp"

namespace et_game {

static std::unordered_map<std::string, std::string> parse_query_string(
    const std::string& uri
) {
    std::unordered_map<std::string, std::string> query_params{};
    
    std::size_t query_start = uri.find('?');
    if(query_start == std::string::npos) {
        return query_params;
    }

    std::string query_str = uri.substr(query_start + 1);
    std::istringstream query_stream(query_str);

    std::string pair{};
    while(std::getline(query_stream, pair, '&')) {
        std::size_t eq_pos = pair.find('=');
        if(eq_pos == std::string::npos) {
            continue;
        }

        query_params[pair.substr(0, eq_pos)] = pair.substr(eq_pos + 1);
    }

    return query_params;
}

WebsocketServer::WebsocketServer(std::uint16_t port, InputQueue& input_queue) 
    : _server(port), _input_queue(input_queue)
{}

void WebsocketServer::run() {
    spdlog::info("Starting WebSocket server on port {}...", _server.getPort());

    _server.setOnClientMessageCallback(
        [this](ConnectionPtr conn, WebSocketRef web_socket, MessagePtr msg) {
            this->message_handler(conn, web_socket, msg);
        }
    );

    auto listener = _server.listen();
    if(!listener.first) {
        spdlog::error("Failed to start WebSocket server: {}", listener.second);
        return;
    }

    _server.start();
    spdlog::info("WebSocket server started successfully on port {}!", _server.getPort());

    _server.wait();
}

void WebsocketServer::send_msg_to_curr(const std::string& msg) {
    std::lock_guard lock(_conn_mutex);
    if(!_curr_websocket) {
        spdlog::warn("No client connected, cannot send message: '{}'", msg);
        return;
    }

    _curr_websocket->send(msg);
}

void WebsocketServer::message_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg) {
    switch(msg->type) {
        case ix::WebSocketMessageType::Open:
            on_open_handler(conn, websocket, msg);
            break;
        
        case ix::WebSocketMessageType::Close:
            on_close_handler(conn, websocket, msg);
            break;
        
        case ix::WebSocketMessageType::Message:
            on_message_handler(conn, websocket, msg);
            break;
        
        case ix::WebSocketMessageType::Error:
            on_error_handler(conn, websocket, msg);
            break;
        
        default:
            spdlog::debug("Received unknown message type from client {}: {}!", conn->getId(), static_cast<int>(msg->type));
            break;
    }
}

void WebsocketServer::on_open_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg) {
    const std::string client_id = conn->getId();
    const std::string uri = msg->openInfo.uri;
    spdlog::info("Client attempting to connect: id={}, uri={}", client_id, uri);

    auto params = parse_query_string(uri);
    std::string user_id = params.count("userId") ? params["userId"] : "";
    std::string mode = params.count("mode") ? params["mode"] : "continue";

    if (user_id.empty()) {
        spdlog::warn("Connection from {} has no userId; rejecting!", client_id);
        websocket.close(1008, "Missing userId query parameter!");

        return;
    }

    bool claimed_conn_slot{};
    std::string existing_client_id{};
    {
        std::lock_guard lock(_conn_mutex);
        if(!_curr_websocket) {
            _curr_websocket = &websocket;
            _curr_client_id = client_id;

            claimed_conn_slot = true;
        }
        else {
            existing_client_id = _curr_client_id;
        }
    }

    if(!claimed_conn_slot) {
        spdlog::warn(
            "Rejecting connection from {}! Client {} has taken the connection slot!",
            client_id, existing_client_id    
        );

        websocket.close(1008, "Server is single-player; another session is active!");
    
        return;
    }

    spdlog::info("Client accepted: id={}, userId={}, mode={}", client_id, user_id, mode);
    _input_queue.push(ConnectEvent{user_id, client_id, mode});
}

void WebsocketServer::on_close_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg) {
    (void) websocket;

    const std::string client_id = conn->getId();
    spdlog::info("Client disconnected: id={}, code={}, reason='{}'!",
        client_id,
        msg->closeInfo.code,
        msg->closeInfo.reason
    );

    bool was_current = false;
    {
        std::lock_guard lock(_conn_mutex);
        if(_curr_client_id == client_id) {
            _curr_websocket = nullptr;
            _curr_client_id.clear();

            was_current = true;
        }
    }

    if(was_current) {
        _input_queue.push(DisconnectEvent{client_id});
    }
}

void WebsocketServer::on_message_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg) {
    (void) websocket;

    const std::string client_id = conn->getId();
    spdlog::debug("Received message from client {}: '{}'", client_id, msg->str);

    bool is_current{};
    {
        std::lock_guard lock(_conn_mutex);
        is_current = (_curr_client_id == client_id);
    }

    if(!is_current) {
        spdlog::debug("Ignoring message from non-current client: {}", client_id);
        return;
    }

    nlohmann::json payload{};
    try {
        payload = nlohmann::json::parse(msg->str);
    }
    catch(const nlohmann::json::parse_error& e) {
        spdlog::warn("Failed to parse message from client {} as JSON: '{}', error: '{}'!", client_id, msg->str, e.what());
        return;
    }

    _input_queue.push(GameEvent{client_id, std::move(payload)});
}

void WebsocketServer::on_error_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg) {
    (void) websocket;
    spdlog::error("Error from client {}: {}", conn->getId(), msg->errorInfo.reason);
}

} // namespace et_game
