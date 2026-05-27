#pragma once
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
namespace et{
    namespace beast = boost::beast;
    namespace websocket = beast::websocket;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    class WebSockets{
        private:
            static net::io_context ioContext;
            static tcp::resolver resolver;
            static websocket::stream<tcp::socket> ws;
        public:
            static void Create();
            static void Connect(const std::string& host, const std::string& port, std::string endpoint);
            static void Send(const std::string& message);
            static std::string Read();
            static void Close();
    };
}