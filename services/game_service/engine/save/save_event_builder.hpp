#ifndef ET_GAME_SAVE_EVENT_BUILDER_HPP
#define ET_GAME_SAVE_EVENT_BUILDER_HPP

#include <string>

#include "engine/save/save_event.hpp"
#include "engine/state/game_state.hpp"

namespace et_game {

SaveEvent build_save_event(const GameState& game_state, const std::string& trigger);

} // namespace et_game

#endif
