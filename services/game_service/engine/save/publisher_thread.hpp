#ifndef ET_GAME_PUBLISHER_THREAD_HPP
#define ET_GAME_PUBLISHER_THREAD_HPP

#include "engine/save/save_publisher.hpp"
#include "engine/util/concurrent_queue.hpp"

namespace et_game {

void run_publisher_thread(SavePublisher& save_publisher, SaveEventQueue& save_queue);

} // namespace et_game

#endif
