#include <queue>
#include <vector>
#include <format>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "world_loader.hpp"

namespace et_game {
using Json = nlohmann::json;

namespace detail {
namespace parsing {
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
    catch(const json::exception& e) {
        throw WorldLoadError(std::format("Failed to parse field '{}' in {}: {}", field_name, ctx, e.what()));
    }
}

Direction parse_direction(const std::string& dir_str) {
    if(dir_str == "N") return Direction::North;
    if(dir_str == "S") return Direction::South;
    if(dir_str == "E") return Direction::East;
    if(dir_str == "W") return Direction::West;

    throw WorldLoadError(std::format("Invalid direction string '{}'! Expected 'N', 'S', 'E', or 'W'", dir_str));
}

WorldConfig parse_config(const Json& json_obj) {
    const std::string ctx = "world config";

    WorldConfig world_cfg{};
    world_cfg.starting_energy   = require_field<float>(json_obj, "startingEnergy", ctx);
    world_cfg.energy_drain_rate = require_field<float>(json_obj, "energyDrain_rate", ctx);
    world_cfg.energy_per_candy  = require_field<float>(json_obj, "energyPerCandy", ctx);
    world_cfg.starting_room_id  = require_field<RoomId>(json_obj, "startRoomId", ctx);

    const Json& spawn_pos_json = require_field<Json>(json_obj, "startSpawn", ctx);
    world_cfg.et_spawn_pos.x = require_field<float>(spawn_pos_json, "x", ctx + " -> startSpawn");
    world_cfg.et_spawn_pos.y = require_field<float>(spawn_pos_json, "y", ctx + " -> startSpawn");

    world_cfg.required_phone_pieces = require_field<std::vector<ItemTypeId>>(
        json_obj,
        "requiredPhonePieces",
        ctx
    );

    return world_cfg;
}

std::unordered_map<ItemTypeId, PickupDef> parse_pickup_defs(const Json& json_obj) {
    if(!json_obj.is_object()) {
        throw WorldLoadError("Invalid pickupDefs format: expected a JSON object!");
    }

    auto parse_pickup_kind = [](const std::string& kind_str) -> PickupDef::Kind {
        if(kind_str == "phonePiece") return PickupDef::Kind::PhonePiece;
        if(kind_str == "candy") return PickupDef::Kind::Candy;

        throw WorldLoadError(std::format("Invalid pickup kind string '{}'! Expected 'phonePiece' or 'candy'", kind_str));
    };

    std::unordered_map<ItemTypeId, PickupDef> pickup_defs{};
    for(auto it = json_obj.begin(); it != json_obj.end(); ++it) {
        const ItemTypeId& pickup_id = it.key();
        const Json& pickup_def_json = it.value();
        const std::string ctx = std::format("pickupDef '{}'", pickup_id);

        PickupDef pickup_def{};
        const std::string pickup_kind_str = require_field<std::string>(pickup_def_json, "kind", ctx);
        pickup_def.kind = parse_pickup_kind(pickup_kind_str);
        pickup_def.display_name = require_field<std::string>(pickup_def_json, "displayName", ctx);

        spdlog::debug("Parsed pickupDef '{}': kind={}, display_name='{}'",
            pickup_id,
            pickup_kind_str,
            pickup_def.display_name
        );

        pickup_defs.emplace(pickup_id, std::move(pickup_def));
    }

    return pickup_defs;
}

std::vector<TileId> decode_tile_grid(
    const Json& tiles_json,
    int width,
    int height,
    const RoomId& room_id
) {
    if(!tiles_json.is_array()) {
        throw WorldLoadError(std::format("Invalid tiles format for room '{}': expected a JSON array!", room_id));
    }

    if(static_cast<int>(tiles_json.size()) != height) {
        throw WorldLoadError(std::format("Invalid tiles format for room '{}': expected {} rows, but got {}!", room_id, height, tiles_json.size()));
    }

    std::vector<TileId> tiles{};
    tiles.reserve(width * height);

    for(int y = 0; y < height; ++y) {
        if(!tiles_json[y].is_string()) {
            throw WorldLoadError(std::format("Invalid tiles format for room '{}': expected row {} to be a JSON string!", room_id, y));
        }

        const std::string row_str = tiles_json[y].get<std::string>();
        if(static_cast<int>(row_str.size()) != width) {
            throw WorldLoadError(std::format(
                "Invalid tiles format for room '{}': expected row {} to have width {} ({} chars), but got {} ({} chars)!", 
                room_id, y, width, row_str.size(), width, row_str.size())
            );
        }

        for(int x = 0; i < width; ++x) {
            const char tile_char = row_str[x];
            if(tile_char < '0' || tile_char > '9') {
                throw WorldLoadError(std::format(
                    "Invalid tile character '{}' at position ({}, {}) in room '{}': expected a digit between '0' and '9'!", 
                    tile_char, x, y, room_id)
                );
            }

            tiles.push_back(static_cast<TileId>(tile_char - '0'));
        }
    }

    return tiles;
}

Exit parse_exit(
    const Json& json_obj,
    const RoomId& room_id,
    std::size_t index
) {
    const std::string ctx = std::format("exit #{} in room '{}'", index, room_id);

    const Json& exit_trigger_json = require_field<Json>(json_obj, "trigger", ctx);
    if(exit_trigger_json.contains("kind")) {
        const std::string kind = require_field<std::string>(exit_trigger_json, "kind", ctx + " -> trigger");
        if(kind != "edge") {
            throw WorldLoadError(std::format("Unsupported exit trigger kind '{}' for {}! Expected 'edge'", kind, ctx));
        }
    }

    const std::string side_str = require_field<std::string>(exit_trigger_json, "side", ctx + " -> trigger");

    Exit exit{};
    exit.edge_direction = parse_direction(side_str);
    exit.destination_room_id = require_field<RoomId>(json_obj, "destinationRoomId", ctx);

    const Json& spawn_pos_json = require_field<Json>(json_obj, "spawnPoint", ctx);
    exit.spawn_pos.x = require_field<float>(spawn_pos_json, "x", ctx + " -> spawnPoint");
    exit.spawn_pos.y = require_field<float>(spawn_pos_json, "y", ctx + " -> spawnPoint");

    spdlog::debug("Parsed exit in room '{}': edge_direction={}, destination_room_id='{}', spawn_pos=({}, {})",
        room_id,
        side_str,
        exit.destination_room_id,
        exit.spawn_pos.x,
        exit.spawn_pos.y
    );

    return exit;
}

PickupSpawn parse_pickup_spawn(
    const Json& json_obj,
    const RoomId& room_id,
    std::size_t index
) {
    const std::string ctx = std::format("pickup spawn #{} in room '{}'", index, room_id);

    PickupSpawn spawn{};
    spawn.id = require_field<PickupId>(json_obj, "id", ctx);
    spawn.type = require_field<ItemTypeId>(json_obj, "type", ctx);
    spawn.pos.x = require_field<int>(json_obj, "x", ctx);
    spawn.pos.y = require_field<int>(json_obj, "y", ctx);

    spdlog::debug("Parsed pickup spawn '{}': type='{}', pos=({}, {})",
        spawn.id,
        spawn.type,
        spawn.pos.x,
        spawn.pos.y
    );

    return spawn;
}

Room parse_room(const RoomId& id, const Json& json_obj) {
    const std::string ctx = std::format("room '{}'", id);

    Room room{};
    room.id = id;
    room.tileset_name = require_field<TilesetName>(json_obj, "tileset", ctx);
    room.width = require_field<int>(json_obj, "width", ctx);
    room.height = require_field<int>(json_obj, "height", ctx);

    if(room.width <= 0 || room.height <= 0) {
        throw WorldLoadError(std::format("Invalid room dimensions for {}: width and height must be positive integers!", ctx));
    }

    room.tiles = decode_tile_grid(
        require_field<std::vector<TileId>>(json_obj, "tiles", ctx),
        room.width, room.height, id
    );

    const Json& exits_json = require_field<Json>(json_obj, "exits", ctx);
    if(!exits_json.is_array()) {
        throw WorldLoadError(std::format("Invalid exits format for {}: expected a JSON array!", ctx));
    }

    for(std::size_t i = 0; i < exits_json.size(); ++i) {
        room.exits.push_back(parse_exit(exits_json[i], id, i));
    }

    const Json& pickup_spawns_json = require_field<Json>(json_obj, "pickupSpawns", ctx);
    if(!pickup_spawns_json.is_array()) {
        throw WorldLoadError(std::format("Invalid pickupSpawns format for {}: expected a JSON array!", ctx));
    }

    for(std::size_t i = 0; i < pickup_spawns_json.size(); ++i) {
        room.pickup_spawns.push_back(parse_pickup_spawn(pickup_spawns_json[i], id, i));
    }

    spdlog::debug("Parsed room '{}': width={}, height={}, tileset='{}', exits={}, pickup spawns={}",
        room.id,
        room.width,
        room.height,
        room.tileset_name,
        room.exits.size(),
        room.pickup_spawns.size()
    );

    return room;
}
} // namespace parsing

namespace validation {
class ValidationErrors {
public:
    void add(const std::string& error) {
        _errors.push_back(error);
    }

    bool empty() const { return _errors.empty(); }
    std::size_t count() { return _errors.size(); }

    void throw_if_has_error() const {
        if(empty()) {
            return;
        }

        std::string formatted_errors = std::format(
            "Failed to validate world! Found {} error(s):\n",
            _errors.size()
        );
        
        for(const auto& error : _errors) {
            formatted_errors += " - " + error + "\n";
        }

        throw WorldLoadError(formatted_errors);
    }

private:
    std::vector<std::string> _errors;
};
} // namespace validation

} // namespace detail

World load_world(const std::string& world_file_path) {
    std::ifstream file(world_file_path);
    if(!file.is_open()) {
        throw WorldLoadError(std::format("Failed to open world file '{}'!", world_file_path));
    }

    Json world_json_root{};
    try {
        file >> world_json_root;
    }
    catch(const Json::exception& error) {
        throw WorldLoadError(std::format("Failed to parse world JSON: {}", error.what()));
    }

    World world{};
    const std::string ctx = "world root";

    world.schema_version = detail::require_field<int>(world_json_root, "schemaVersion", ctx);
    if(world.schema_version != 1) {
        throw WorldLoadError(std::format("Unsupported world schema version {}! Expected version 1.", world.schema_version));
    }

    world.config = detail::parse_config(
        detail::require_field<Json>(world_json_root, "config", ctx)
    );

    world.pickup_defs = detail::parse_pickup_defs(
        detail::require_field<Json>(world_json_root, "pickupTypes", ctx)
    );

    const Json& rooms_json = detail::require_field<Json>(world_json_root, "rooms", ctx);
    if(!rooms_json.is_object()) {
        throw WorldLoadError("Invalid rooms format: expected a JSON object!");
    }

    for(auto it = rooms_json.begin(); it != rooms_json.end(); ++it) {
        const RoomId& room_id = it.key();
        const Json& room_json = it.value();

        world.rooms.emplace(room_id, detail::parse_room(room_id, room_json));
    }

    detail::ValidationErrors validation_errors{};
    validation_errors.throw_if_has_error();

    spdlog::info("Successfully loaded world from '{}'!", world_file_path);
    spdlog::debug("Loaded world config: starting_energy={}, energy_drain_rate={}, energy_per_candy={}, starting_room_id='{}', required_phone_pieces={}",
        world.config.starting_energy,
        world.config.energy_drain_rate,
        world.config.energy_per_candy,
        world.config.starting_room_id,
        fmt::join(world.config.required_phone_pieces, ", ")
    );

    return world;
}

void validate_tilesets(
    const World& world,
    const std::unordered_map<TilesetName, Tileset>& tilesets
) {

}

} // namespace et_game
