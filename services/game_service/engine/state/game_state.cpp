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

GameState::GameState(const std::string& user_id, const Json& json) 
: user_id(user_id)
{
    if(!json.contains("player")) {
        throw std::runtime_error("Save game_state_json missing 'player' field!");
    }

    const auto& player_game_state_json = json.at("player");
    player.current_room_id = player_game_state_json.at("currentRoomId").get<std::string>();
    player.local_pos.x = player_game_state_json.at("localX").get<float>();
    player.local_pos.y = player_game_state_json.at("localY").get<float>();

    energy = json.at("energy").get<float>();
    candy_count = json.at("candyCount").get<int>();

    if(json.contains("phonePiecesCollected")) {
        for(const auto& piece : json.at("phonePiecesCollected")) {
            collected_phone_pieces.push_back(piece.get<std::string>());
        }
    }
    
    if(json.contains("collectedPickups")) {
        for(const auto& pickup_id : json.at("collectedPickups")) {
            collected_pickups.insert(pickup_id.get<std::string>());
        }
    }

    const std::string status_str = json.value("status", "playing");

    auto it = STATUSES.find(status_str);
    if (it == STATUSES.end()) {
        spdlog::warn("Unknown status '{}' in save for '{}', defaulting to Playing", status_str, user_id);
        status = GameStatus::Playing;
    } 
    else {
        status = it->second;
    }

    spdlog::info(
        "Deserialized save for user '{}': room='{}', pos=({}, {}), energy={}, candy={}, pieces={}, depleted_pickups={}",
        user_id, player.current_room_id,
        player.local_pos.x, player.local_pos.y,
        energy, candy_count, collected_phone_pieces.size(), collected_pickups.size()
    );
}

Json GameState::to_json() const {
    Json game_state_json{};

    game_state_json["player"] = {
        {"currentRoomId", player.current_room_id},
        {"localX", player.local_pos.x},
        {"localY", player.local_pos.y}
    };

    game_state_json["energy"] = energy;
    game_state_json["candyCount"] = candy_count;
    game_state_json["phonePiecesCollected"] = collected_phone_pieces;
    
    game_state_json["collectedPickups"] = Json::array();
    for(const auto& id: collected_pickups) {
        game_state_json["collectedPickups"].push_back(id);
    }

    switch (status) {
        case GameStatus::Playing: game_state_json["status"] = "playing"; break;
        case GameStatus::Won:     game_state_json["status"] = "won"; break;
        case GameStatus::Lost:    game_state_json["status"] = "lost"; break;
    }

    return game_state_json;
}

} // namespace et_game
