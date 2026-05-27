#ifndef ET_GAME_GAME_LOOP_HPP
#define ET_GAME_GAME_LOOP_HPP

#include "models/world.hpp"
#include "engine/state/game_state.hpp"
#include "engine/util/concurrent_queue.hpp"

namespace et_game {

void run_game_loop(
    const World& world,
    InputQueue& input_queue,
    OutputQueue& output_queue
);

void tick(
    GameState& game_state,
    const World& world,
    double dt,
    OutputQueue& output_queue
);

} // namespace et_game

#endif
