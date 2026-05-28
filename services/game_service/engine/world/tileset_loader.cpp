#include <format>
#include <fstream>
#include <stdexcept>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "engine/world/tileset_loader.hpp"
#include "models/tileset.hpp"

namespace et_game {

using Json = nlohmann::json;

namespace {

TileId parse_tile_id_char(const std::string& id) {
    try {
        std::size_t parsing_pos{};
        int parsed_id = std::stoi(id, &parsing_pos);

        if(parsing_pos != id.size()) {
            throw TilesetError(std::format("Failed to parse tile id '{}'! Error: ID has trailing characters!", id));
        }

        if(parsed_id < 0 || parsed_id > 9) {
            throw TilesetError(std::format("Failed to parse tile id '{}'! Error: id must be 0-9!", id));
        }

        return static_cast<TileId>(parsed_id);
    }
    catch(const std::invalid_argument&) {
        throw TilesetError(std::format("Failed to parse tile id '{}'! Error: id is NOT a valid integer!", id));
    }
    catch(const std::exception& e) {
        throw TilesetError(std::format("Failure to parse tile id '{}'! Error: {}", id, e.what()));
    }
}

} // anonymous namespace

Tileset load_tileset(
    const std::string& tileset_path,
    const std::string& tileset_name
) {
    std::ifstream tileset_file(tileset_path);
    if(!tileset_file.is_open()) {
        throw TilesetError(std::format("Failed to open tileset file '{}'!", tileset_path));
    }

    Json tileset_json{};

    try {
        tileset_file >> tileset_json;
    }
    catch (const Json::parse_error& e) {
        throw TilesetError(std::format("Failed to parse tileset '{}': {}", tileset_path, e.what()));
    }

    Tileset tileset{};
    tileset.name = tileset_name;
    tileset.tile_size = tileset_json.value("tileSize", 0);
    tileset.tiles_per_row = tileset_json.value("tilesPerRow", 0);

    if(!tileset_json.contains("tiles") || ! tileset_json["tiles"].is_object()) {
        throw TilesetError(std::format("Tileset '{}' missing 'tiles' object", tileset_name));
    }

    for(const auto& [tile_id_char, tile_json] : tileset_json["tiles"].items()) {
        TileId tile_id = parse_tile_id_char(tile_id_char);

        TileDef tile_def{};
        tile_def.walkable = tile_json.value("walkable", false);
        tile_def.name = tile_json.value("name", std::string{"Unknown tileset name!"});

        auto [_, inserted] = tileset.tile_defs.emplace(tile_id, std::move(tile_def));
        if(!inserted) {
            throw TilesetError(std::format(
                "Tileset '{}' has duplicate tile ID {}", tileset_name, tile_id
            ));
        }
    }

    if(tileset.tile_defs.empty()) {
        throw TilesetError(std::format("Tileset '{}' has no tile definitions", tileset_name));
    }

    spdlog::debug("Loaded tileset '{}' with {} tile definitions", tileset_name, tileset.tile_defs.size());

    return tileset;
}

} // namespace et_game
