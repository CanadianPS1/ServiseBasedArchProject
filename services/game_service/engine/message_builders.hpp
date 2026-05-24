#ifndef ET_GAME_MESSAGE_BUILDERS_HPP
#define ET_GAME_MESSAGE_BUILDERS_HPP

#include "game_state.hpp"
#include "../models/world.hpp"

namespace et_game {

std::string build_room_loaded(const World& world, const GameState& game_state);

std::string build_state_update(const GameState& game_state);

} // namespace et_game

#endif
