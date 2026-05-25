#ifndef ET_GAME_GAME_LOGIC_HPP
#define ET_GAME_GAME_LOGIC_HPP

#include "engine/state/game_state.hpp"
#include "engine/network/output_queue.hpp"
#include "models/world.hpp"

namespace et_game {

void apply_movement_event(GameState& game_state, double dt);

bool handle_exit_transitions(
    GameState& game_state,
    const World& world,
    OutputQueue& output_queue
);

void handle_pickup_collection(GameState& game_state, const World& world);

void drain_energy(GameState& game_state, const World& world, double dt);

void check_win_lose(GameState& game_state, const World& world);

} // namespace et_game

#endif