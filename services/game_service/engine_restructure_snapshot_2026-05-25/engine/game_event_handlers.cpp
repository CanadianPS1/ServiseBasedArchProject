#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "game_event_handlers.hpp"

namespace et_game {

namespace {

const std::unordered_map<std::string, InputHandlerFn> HANDLERS = {
    {"input_move", &handle_move_event},
};

} // anonymous namespace

void dispatch_game_event(GameState& game_state, const Json& json_payload) {
    if(!json_payload.contains("type")) {
        spdlog::warn("Game event missing 'type' field!");
        return;
    }

    const std::string input_type = json_payload["type"].get<std::string>();
    auto it = HANDLERS.find(input_type);
    if(it == HANDLERS.end()) {
        spdlog::warn("Invalid game event type: {}", input_type);
        return;
    }

    it->second(game_state, json_payload);
}

void handle_move_event(GameState& game_state, const Json& json_payload) {
    const std::string direction = json_payload.value("direction", "");
    const bool pressed = json_payload.value("pressed", false);

    if (direction == "N")      game_state.input_state.move_north = pressed;
    else if (direction == "S") game_state.input_state.move_south = pressed;
    else if (direction == "E") game_state.input_state.move_east  = pressed;
    else if (direction == "W") game_state.input_state.move_west  = pressed;
    else spdlog::warn("Unknown direction '{}' in input_move", direction);
}

} // namespace et_game
