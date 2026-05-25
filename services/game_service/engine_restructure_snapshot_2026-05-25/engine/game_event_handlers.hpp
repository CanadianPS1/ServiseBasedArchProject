#ifndef ET_GAME_EVENT_HANDLERS_HPP
#define ET_GAME_EVENT_HANDLERS_HPP

#include "game_state.hpp"

#include <nlohmann/json.hpp>

namespace et_game {

using Json = nlohmann::json;
using InputHandlerFn = void(*)(GameState&, const Json&);

void dispatch_game_event(GameState& game_state, const Json& json_payload);

void handle_move_event(GameState& game_state, const Json& json_payload);

} // namespace et_game

#endif
