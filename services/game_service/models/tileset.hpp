#ifndef ET_GAME_SERVICE_TILESET_HPP
#define ET_GAME_SERVICE_TILESET_HPP

#include <format>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "types.hpp"

namespace et_game {

struct TileDef {
    bool is_walkable = false;
    std::string display_name = "Unknown Tile";
};

struct Tileset {
    TilesetName name;
    int tile_size = 0;
    int tiles_per_row = 0;

    std::unordered_map<TileId, TileDef> tile_defs;

    const TileDef& get_tiledef_by_id(TileId id) const {
        auto it = tile_defs.find(id);
        if (it == tile_defs.end()) {
            throw std::runtime_error(std::format("Tile ID {} not found in tileset '{}'", id, name));
        }

        return it->second;
    }

    bool is_walkable(TileId id) const {
        return get_tiledef_by_id(id).is_walkable;
    }
};

} // namespace et_game

#endif
