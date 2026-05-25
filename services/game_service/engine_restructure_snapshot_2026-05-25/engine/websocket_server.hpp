#ifndef ET_GAME_WEBSOCKET_SERVER_HPP
#define ET_GAME_WEBSOCKET_SERVER_HPP

#include <memory>
#include <cstdint>

#include <ixwebsocket/IXWebSocketServer.h>

#include "input_queue.hpp"

namespace et_game {

class WebsocketServer {
public:
    WebsocketServer(std::uint16_t port, InputQueue& InputQueue);

    void run();

    void send_msg_to_curr(const std::string& msg);

private:
    using ConnectionPtr = std::shared_ptr<ix::ConnectionState>;
    using MessagePtr = const ix::WebSocketMessagePtr&;
    using WebSocketRef = ix::WebSocket&;

    void message_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg);
    void on_open_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg);
    void on_close_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg);
    void on_message_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg);
    void on_error_handler(ConnectionPtr conn, WebSocketRef websocket, MessagePtr msg);

    ix::WebSocketServer _server{};
    InputQueue& _input_queue;

    std::mutex _conn_mutex{};
    ix::WebSocket* _curr_websocket{};
    std::string _curr_client_id{};
};

} // namespace et_game

#endif
