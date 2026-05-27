#ifndef ET_GAME_SENDER_THREAD_HPP
#define ET_GAME_SENDER_THREAD_HPP

#include <spdlog/spdlog.h>

#include "engine/util/concurrent_queue.hpp"
#include "engine/network/websocket_server.hpp"

namespace et_game {

inline void run_sender_thread(WebsocketServer& websocket_server, OutputQueue& output_queue) {
    spdlog::info("Sender thread starting...");

    while(true) {
        auto msg = output_queue.pop();
        if(!msg.has_value()) {
            spdlog::info("Sender thread shutting down...");
            break;
        }

        websocket_server.send_msg_to_curr(*msg);
    }
}

} // namespace et_game

#endif
