#pragma once
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <queue>
namespace et{
	namespace beast = boost::beast;
	namespace websocket = beast::websocket;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	class WebSockets{
		private:
			static net::io_context ioContext;
			static std::unique_ptr<tcp::resolver> resolver;
			static std::unique_ptr<websocket::stream<tcp::socket>> ws;
		public:
			static void Create();
			static bool Connect(const std::string& host,const std::string& port,const std::string& endpoint);
			static bool Send(const std::string& message);
			static std::string Read();
			static void Close();
            static std::queue<std::string> messages;
            static void OnRead();       
	};
}