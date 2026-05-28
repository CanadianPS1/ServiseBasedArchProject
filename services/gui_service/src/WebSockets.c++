#include "WebSockets.hpp"
#include <iostream>
namespace et{
	net::io_context WebSockets::ioContext{};
	std::unique_ptr<tcp::resolver> WebSockets::resolver{};
	std::unique_ptr<websocket::stream<tcp::socket>> WebSockets::ws{};
	void WebSockets::Create(){
		resolver = std::make_unique<tcp::resolver>(ioContext);
		ws = std::make_unique<websocket::stream<tcp::socket>>(ioContext);
	}
	bool WebSockets::Connect(const std::string& host,const std::string& port,const std::string& endpoint){
        try{
            if(!resolver) resolver = std::make_unique<tcp::resolver>(ioContext);
            if(!ws) ws = std::make_unique<websocket::stream<tcp::socket>>(ioContext);
            if(ws->is_open()){
                beast::error_code ec;
                ws->close(websocket::close_code::normal,ec);
            }
            ws = std::make_unique<websocket::stream<tcp::socket>>(ioContext);
            auto const results = resolver->resolve(host,port);
            net::connect(ws->next_layer(),results.begin(),results.end());
            ws->handshake(host,endpoint);
            OnRead();
            return true;
        }catch(const std::exception& e){
            std::cerr<<"WebSocket Connect Error: "<<e.what()<<std::endl;
            return false;
        }
    }
	bool WebSockets::Send(const std::string& message){
		try{
			if(!ws || !ws->is_open()) return false;
			ws->write(net::buffer(message));
			return true;
		}catch(const std::exception& e){
			std::cerr<<"WebSocket Send Error: "<<e.what()<<std::endl;
			return false;
		}
	}
	std::string WebSockets::Read(){
        if(!ws || !ws->is_open()) return "";
            beast::flat_buffer buffer;
            boost::beast::error_code ec;
            ws->read(buffer,ec);
            if(ec == boost::asio::error::would_block) return "";
            if(ec) return "";
            return beast::buffers_to_string(buffer.data());
    }
	void WebSockets::Close(){
		try{
			if(!ws || !ws->is_open()) return;
			beast::error_code ec;
			ws->close(websocket::close_code::normal,ec);
			if(ec) std::cerr<<"WebSocket Close Error: "<<ec.message()<<std::endl;
		}catch(const std::exception& e){std::cerr<<"WebSocket Close Exception: "<<e.what()<<std::endl;}
	}
    void WebSockets::OnRead(){
        if(!ws || !ws->is_open()) return;
        auto buffer=std::make_shared<beast::flat_buffer>();
        ws->async_read(*buffer, [buffer](beast::error_code ec,std::size_t){
            if(!ec){
                messages.push(beast::buffers_to_string(buffer->data()));
                OnRead();
            }
        });
    }
}