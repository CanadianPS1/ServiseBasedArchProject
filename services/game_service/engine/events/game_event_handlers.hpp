#ifndef ET_GAME_EVENT_HANDLERS_HPP
#define ET_GAME_EVENT_HANDLERS_HPP

#include <nlohmann/json.hpp>

#include "models/world.hpp"
#include "engine/state/game_state.hpp"
#include "engine/util/concurrent_queue.hpp"

namespace et_game {

using Json = nlohmann::json;
using InputHandlerFn = void(*)(GameState&, const World&, SaveEventQueue&, const Json&);

void dispatch_game_event(
    GameState& game_state, 
    const World& world,
    SaveEventQueue& save_queue,
    const Json& json_payload
);

void handle_move_event(
    GameState& game_state,
    const World& world,
    SaveEventQueue& save_queue,
    const Json& json_payload
);

void handle_consume_candy_event(
    GameState& game_state,
    const World& world,
    SaveEventQueue& save_queue,
    const Json& json_payload
);

} // namespace et_game

#endif
