#include <format>
#include <fstream>
#include <unordered_set>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <spdlog/fmt/bundled/format.h>

#include "engine/world/world_loader.hpp"
#include "engine/world/world_parsing.hpp"
#include "engine/world/tileset_loader.hpp"
#include "engine/world/world_validation.hpp"

namespace et_game {

using Json = nlohmann::json;

World load_world(const std::string& world_file_path, const std::string& tileset_dir_path) {
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
    const std::string ctx = "root";

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

    std::unordered_set<TilesetName> needed_tilesets{};
    for(const auto& [_, room] : world.rooms) {
        needed_tilesets.insert(room.tileset_name);
    }

    for(const TilesetName& tileset_name : needed_tilesets) {
        std::string tileset_path = std::format("{}/{}.json", tileset_dir_path, tileset_name);
        spdlog::debug("Loading tileset '{}' from '{}'", tileset_name, tileset_path);
        world.tilesets.emplace(tileset_name, load_tileset(tileset_path, tileset_name));
    }

    detail::ValidationErrors validation_errors{};
    detail::validate_config(world, validation_errors);
    detail::validate_tilesets(world, validation_errors);
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

} // namespace et_game
