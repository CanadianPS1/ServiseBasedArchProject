#include <queue>
#include <vector>
#include <format>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "world_loader.hpp"
#include "world_parsing.hpp"
#include "world_validation.hpp"

namespace et_game {
using Json = nlohmann::json;

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
    detail::validate_config(world, validation_errors);
    detail::validate_required_pieces(world, validation_errors);
    detail::validate_exits(world, validation_errors);
    detail::validate_pickup_spawns(world, validation_errors);
    if(validation_errors.empty()) {
        detail::validate_room_reachability(world, validation_errors);
    }

    validation_errors.throw_if_any_error();

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
    detail::ValidationErrors errors{};

    for(const auto& [room_id, room] : world.rooms) {
        auto tileset_it = tilesets.find(room.tileset_name);
        if(tileset_it == tilesets.end()) {
            errors.add(std::format(
                "Room '{}' references undefined tileset '{}'",
                room_id, room.tileset_name
            ));
            continue;
        }

        const Tileset& tileset = tileset_it->second;
        std::unordered_set<TileId> seen_tile_ids{};

        for(int y = 0; y < room.height; ++y) {
            for(int x = 0; x < room.width; ++x) {
                TileId tile_id = room.tile_at(x, y);
                if(tileset.tile_defs.contains(tile_id)) { continue; }
                if(seen_tile_ids.contains(tile_id)) { continue; }

                seen_tile_ids.insert(tile_id);
                errors.add(std::format(
                    "Room '{}' uses tile ID {} at position ({}, {}) that is not defined in tileset '{}'",
                    room_id, tile_id, x, y, room.tileset_name
                ));
            }
        }
    }

    errors.throw_if_any_error();
}
} // namespace et_game
