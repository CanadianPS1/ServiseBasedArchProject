#ifndef ET_GAME_WEBSOCKET_SERVER_HPP
#define ET_GAME_WEBSOCKET_SERVER_HPP

#include <memory>
#include <cstdint>
#include <ixwebsocket/IXWebSocketServer.h>

namespace et_game {
class WebsocketServer {
public:
    explicit WebsocketServer(std::uint16_t port);

    void run();

private:
    using ConnectionPtr = std::shared_ptr<ix::ConnectionState>;
    using MessagePtr = const ix::WebSocketMessagePtr&;
    using WebSocketPtr = ix::WebSocket&;

    void message_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg);;

    void on_open_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg);
    void on_close_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg);
    void on_message_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg);
    void on_error_handler(ConnectionPtr conn, WebSocketPtr web_socket, MessagePtr msg);

private:
    ix::WebSocketServer _server;
};

} // namespace et_game

#endif
