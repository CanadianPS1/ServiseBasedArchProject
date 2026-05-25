#include <nlohmann/json.hpp>

#include "message_builders.hpp"

namespace et_game {

using Json = nlohmann::json;

namespace {
Json build_active_pickups(const Room& room, const GameState& game_state) {
    Json active_pickups = Json::array();
    for(const auto& pickup_spawn : room.pickup_spawns) {
        if(!game_state.is_pickup_active(pickup_spawn.id)) {
            continue;
        }

        active_pickups.push_back({
            {"id", pickup_spawn.id},
            {"type", pickup_spawn.type},
            {"x", pickup_spawn.pos.x},
            {"y", pickup_spawn.pos.y}
        });
    }

    return active_pickups;
}


} // anonymous namespace

std::string build_room_loaded(
    const World& world,
    const GameState& game_state,
    std::optional<Direction> arrived_from
) {
    const Room& room = world.get_room_by_id(game_state.player.current_room_id);

    Json root_json{};
    root_json["type"] = "room_loaded";

    auto& room_json = root_json["room"];
    room_json["id"] = room.id;
    room_json["tileset"] = room.tileset_name;
    room_json["width"]  = room.width;
    room_json["height"] = room.height;

    auto tiles_json = Json::array();
    for(int y = 0; y < room.height; ++y) {
        std::string tile_row{};
        tile_row.reserve(room.width);

        for(int x = 0; x < room.width; ++x) {
            tile_row.push_back(static_cast<char>('0' + room.tile_at(x, y)));
        }

        tiles_json.push_back(std::move(tile_row));
    }

    room_json["tiles"] = std::move(tiles_json);

    auto active_pickups = Json::array();
    for(const auto& pickup_spawn : room.pickup_spawns) {
        if(!game_state.is_pickup_active(pickup_spawn.id)) {
            continue;
        }

        active_pickups.push_back({
            {"id", pickup_spawn.id},
            {"type", pickup_spawn.type},
            {"x", pickup_spawn.pos.x},
            {"y", pickup_spawn.pos.y}
        });
    }

    room_json["activePickups"] = std::move(active_pickups);

    root_json["player"]["x"] = game_state.player.local_pos.x;
    root_json["player"]["y"] = game_state.player.local_pos.y;


    if(arrived_from.has_value()) {
        root_json["arrivedFrom"] = direction_to_string(*arrived_from);
    }

    return root_json.dump();
}

std::string build_state_update(const World& world, const GameState& game_state) {
    const Room& room = world.get_room_by_id(game_state.player.current_room_id);

    Json root_json{};
    root_json["type"] = "state_update";
    root_json["player"]["x"] = game_state.player.local_pos.x;
    root_json["player"]["y"]=  game_state.player.local_pos.y;
    root_json["energy"] = game_state.energy;
    root_json["phonePieceCount"] = static_cast<int>(game_state.collected_phone_pieces.size());
    root_json["candyCount"] = game_state.candy_count;
    root_json["activePickups"] = build_active_pickups(room, game_state);

    return root_json.dump();
}

std::string build_game_over(const GameState& game_state) {
    Json root_json{};
    root_json["type"] = "game_over";
    root_json["outcome"] = game_state.status == GameStatus::Won ? "won" : "lost";

    return root_json.dump();
}

} // namespace et_game
