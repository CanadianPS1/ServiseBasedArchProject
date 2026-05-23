#ifndef ET_GAME_GAME_LOOP_HPP
#define ET_GAME_GAME_LOOP_HPP

#include "input_queue.hpp"
#include "output_queue.hpp"
#include "../models/world.hpp"

namespace et_game {

void run_game_loop(
    const World& world,
    InputQueue& input_queue,
    OutputQueue& output_queue
);

} // namespace et_game

#endif
