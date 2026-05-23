#include <unordered_map>
#include <spdlog/spdlog.h>

#include "websocket_server.hpp"

namespace et_game {

static std::unordered_map<std::string, std::string> parse_query_string(
    const std::string& uri
) {
    std::unordered_map<std::string, std::string> query_params{};
    
    std::size_t query_start = uri.find('?');
    if(query_start = std::string::npos) {
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

void WebsocketServer::message_handler(ConnectionPtr conn, WebSocketRef web_socket, MessagePtr msg) {
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
            spdlog::debug("Received unknown message type from client {}: {}!", conn->getId(), static_cast<int>(msg->type));
            break;
    }
}

void WebsocketServer::on_open_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg) {
    const std::string client_id = conn->getId();
    const std::string uri = msg->openInfo.uri;

    spdlog::info("Client connected: id={}, uri='{}'", client_id, uri);

    {
        std::lock_guard lock(_conn_mutex);
        if(_curr_websocket) {
            spdlog::warn(
                "A client is already connected (id={}), new connection (id={}) will not receive messages!",
                _curr_client_id, client_id
            );

            websocket.close(1008, "Server is single-player only! Only one client can be connected at a time!");
            return;
        }
        
        _curr_websocket = &websocket;
        _curr_client_id = client_id;
    }

    auto query_params = parse_query_string(uri);
    std::string user_id = query_params.count("userID") ? query_params["userID"] : "unknown";
    std::string mode = query_params.count("mode") ? query_params["mode"] : "continue";

    if(user_id.empty()) {
        spdlog::warn("Client {} did not provide a userID, defaulting to 'unknown' and rejecting!", client_id);
        websocket.close(1008, "Invalid client request: userID is required!");
        
        std::lock_guard lock(_conn_mutex);
        _curr_websocket = nullptr;
        _curr_client_id.clear();
        
        return;
    }

    spdlog::info("Client {} has userID '{}', mode '{}'", client_id, user_id, mode);
    _input_queue.push(ConnectEvent{user_id, client_id, mode});
}

void WebsocketServer::on_close_handler(ConnectionPtr conn, WebSocketRef web_socket, MessagePtr msg) {
    const std::string client_id = conn->getId();
    spdlog::info("Client disconnected: id={}, code={}, reason='{}'",
        client_id,
        msg->closeInfo.code,
        msg->closeInfo.reason
    );

    {
        std::lock_guard lock(_conn_mutex);
        if(_curr_client_id == client_id) {
            _curr_websocket = nullptr;
            _curr_client_id.clear();

            _input_queue.push(DisconnectEvent{client_id});
        }
    }
}

void WebsocketServer::on_message_handler(ConnectionPtr conn, WebSocketRef web_socket, MessagePtr msg) {
    const std::string client_id = conn->getId();
    spdlog::debug("Received message from client {}: '{}'", client_id, msg->str);

    {
        std::lock_guard lock(_conn_mutex);
        if(_curr_client_id != client_id) {
            spdlog::warn(
                "Received message from client {}, but current client is {}! Ignoring message: '{}'",
                client_id, _curr_client_id, msg->str
            );
            return;
        }
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

void WebsocketServer::on_error_handler(ConnectionPtr conn, WebSocketRef web_socket, MessagePtr msg) {
    spdlog::error("Error from client {}: {}", conn->getId(), msg->errorInfo.reason);
}

} // namespace et_game
