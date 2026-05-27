#include "WebSockets.hpp"
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_buffer.hpp>
namespace et{
    net::io_context WebSockets::ioContext{};
    tcp::resolver WebSockets::resolver{ioContext};
    websocket::stream<tcp::socket> WebSockets::ws{ioContext};
    void WebSockets::Connect(const std::string& host, const std::string& port, std::string endpoint){
        auto const results = resolver.resolve(host, port);
        net::connect(ws.next_layer(), results.begin(), results.end());\
        boost::beast::error_code ec;
        ws.handshake(host, endpoint);
    }
    void WebSockets::Send(const std::string& message){
        ws.write(net::buffer(message));
    }
    std::string WebSockets::Read(){
        boost::beast::flat_buffer buffer;
        ws.read(buffer);
        return boost::beast::buffers_to_string(buffer.data());
    }
    void WebSockets::Close(){
        ws.close(websocket::close_code::normal);
    }
}