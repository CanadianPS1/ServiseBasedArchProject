#include <stdexcept>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "engine/state/game_state.hpp"

namespace et_game {

static const std::unordered_map<std::string, GameStatus> STATUSES = {
    {"playing", GameStatus::Playing},
    {"won", GameStatus::Won},
    {"lost", GameStatus::Lost}
};

GameState::GameState(const std::string& user_id, const World& world) 
:
    user_id(user_id),
    energy(world.config.starting_energy),
    candy_count(0),
    status(GameStatus::Playing)
{
    player.current_room_id = world.config.starting_room_id;
    player.local_pos = world.config.et_spawn_pos;
    player.facing = Direction::South;

    spdlog::info(
        "Created new game state for user '{}': room='{}', spawn=({}, {})",
        user_id, player.current_room_id,
        player.local_pos.x, player.local_pos.y
    );
}

GameState::GameState(const std::string& user_id, const Json& save_json) 
: user_id(user_id)
{
    if(!save_json.contains("player")) {
        throw std::runtime_error("Save JSON missing 'player' field!");
    }

    const auto& player_json = save_json.at("player");
    player.current_room_id = player_json.at("currentRoomId").get<std::string>();
    player.local_pos.x = player_json.at("localX").get<float>();
    player.local_pos.y = player_json.at("localY").get<float>();

    energy = save_json.at("energy").get<float>();
    candy_count = save_json.at("candyCount").get<int>();

    if(save_json.contains("collectedPhonePieces")) {
        for(const auto& piece : save_json.at("collectedPhonePieces")) {
            collected_phone_pieces.push_back(piece.get<std::string>());
        }
    }

    const std::string status_str = save_json.value("status", "playing");

    auto it = STATUSES.find(status_str);
    if (it == STATUSES.end()) {
        spdlog::warn("Unknown status '{}' in save for '{}', defaulting to Playing", status_str, user_id);
        status = GameStatus::Playing;
    } 
    else {
        status = it->second;
    }

    spdlog::info(
        "Deserialized save for user '{}': room='{}', pos=({}, {}), energy={}, candy={}, pieces={}",
        user_id, player.current_room_id,
        player.local_pos.x, player.local_pos.y,
        energy, candy_count, collected_phone_pieces.size()
    );
}

} // namespace et_game
