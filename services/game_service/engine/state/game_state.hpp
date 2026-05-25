#ifndef ET_GAME_SERVICE_GAME_STATE_H
#define ET_GAME_SERVICE_GAME_STATE_H

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "models/types.hpp"
#include "models/world.hpp"

namespace et_game {

using Json = nlohmann::json;

enum class GameStatus {
    Won,
    Lost,
    Playing,
    Paused,
};

struct PlayerState {
    Vec2 local_pos{};
    RoomId current_room_id{};
    Direction facing{Direction::South};
};

struct InputState {
    bool move_north{false};
    bool move_south{false};
    bool move_east{false};
    bool move_west{false};
};

struct GameState {
    std::string user_id;

    PlayerState player;
    float energy = 0.0f;

    std::vector<ItemTypeId> collected_phone_pieces;
    int candy_count = 0;

    std::unordered_set<PickupId> collected_pickups;

    GameStatus status = GameStatus::Playing;

    InputState input_state;

    GameState() = default;

    GameState(const std::string& user_id, const World& world);
    GameState(const std::string& user_id, const Json& save_json);

    bool is_playing() const {
        return status == GameStatus::Playing;
    }

    bool is_resumable() const {
        return status == GameStatus::Playing || status == GameStatus::Paused;
    }

    bool is_pickup_active(const PickupId& pickup_id) const {
        return collected_pickups.find(pickup_id) == collected_pickups.end();
    }

    const Room& current_room(const World& world) const {
        return world.get_room_by_id(player.current_room_id);
    }

    bool has_won(const World& world) const {
        if(collected_phone_pieces.size() < world.config.required_phone_pieces.size()) {
            return false;
        }

        for(const auto& required_piece : world.config.required_phone_pieces) {
            if(std::find(collected_phone_pieces.begin(), collected_phone_pieces.end(), required_piece) == collected_phone_pieces.end()) {
                return false;
            }
        }

        return true;
    }
};

} // namespace et_game

#endif
