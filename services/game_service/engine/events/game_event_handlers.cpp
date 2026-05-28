#include <string>
#include <algorithm>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "engine/util/concurrent_queue.hpp"
#include "engine/save/save_event_builder.hpp"
#include "engine/events/game_event_handlers.hpp"

namespace et_game {

const static std::unordered_map<std::string, InputHandlerFn> HANDLERS = {
    {"input_move", &handle_move_event},
    {"input_consume_candy", &handle_consume_candy_event},
};

void dispatch_game_event(
    GameState& game_state,
    const World& world,
    SaveEventQueue& save_queue,
    const Json& json_payload
) {
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

    it->second(game_state, world, save_queue, json_payload);
}

void handle_move_event(
    GameState& game_state,
    const World& world,
    SaveEventQueue& save_queue,
    const Json& json_payload
) {
    (void) world;
    (void) save_queue;

    const std::string direction = json_payload.value("direction", "");
    const bool pressed = json_payload.value("pressed", false);

    if (direction == "N")      game_state.input_state.move_north = pressed;
    else if (direction == "S") game_state.input_state.move_south = pressed;
    else if (direction == "E") game_state.input_state.move_east  = pressed;
    else if (direction == "W") game_state.input_state.move_west  = pressed;
    else spdlog::warn("Unknown direction '{}' in input_move", direction);
}

void handle_consume_candy_event(
    GameState& game_state,
    const World& world,
    SaveEventQueue& save_queue,
    const Json& json_payload
) {
    (void) json_payload;

    if(!game_state.is_playing()) {
        return;
    }

    if(game_state.candy_count <= 0) {
        spdlog::debug("Ignoring input_consume_candy: no candy available");
        return;
    }

    game_state.candy_count--;
    game_state.energy = std::min(
        game_state.energy + world.config.energy_per_candy,
        world.config.starting_energy
    );

    save_queue.push(build_save_event(game_state, "candy_consumed"));
}

} // namespace et_game
