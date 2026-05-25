#ifndef ET_GAME_WORLD_PARSING_HPP
#define ET_GAME_WORLD_PARSING_HPP

#include <string>
#include <vector>
#include <format>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "engine/world/world_loader.hpp"
#include "models/types.hpp"
#include "models/world.hpp"

namespace et_game::detail {

using Json = nlohmann::json;

template<typename T>
T require_field(
    const Json& json_obj,
    const std::string& field_name,
    const std::string& ctx
) {
    if(!json_obj.contains(field_name)) {
        throw WorldLoadError(std::format("Missing required field '{}' in {}", field_name, ctx));
    }

    try {
        return json_obj.at(field_name).get<T>();
    }
    catch(const Json::exception& e) {
        throw WorldLoadError(std::format("Failed to parse field '{}' in {}: {}", field_name, ctx, e.what()));
    }
}

WorldConfig parse_config(const Json& json_obj);
Direction parse_direction(const std::string& dir_str);
Room parse_room(const RoomId& id, const Json& json_obj);
Exit parse_exit(const Json& json_obj, const RoomId& room_id, std::size_t index);
std::unordered_map<ItemTypeId, PickupDef> parse_pickup_defs(const Json& json_obj);
PickupSpawn parse_pickup_spawn(const Json& json_obj, const RoomId& room_id, std::size_t index);
std::vector<TileId> decode_tile_grid(const Json& json_obj, int width, int height, const RoomId& room_id);

} // namespace et_game::detail

#endif
