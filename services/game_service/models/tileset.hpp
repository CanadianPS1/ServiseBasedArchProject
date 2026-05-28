#ifndef ET_GAME_SERVICE_TILESET_HPP
#define ET_GAME_SERVICE_TILESET_HPP

#include <format>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "types.hpp"

namespace et_game {

struct TilesetError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct TileDef {
    bool walkable{false};
    std::string name{"Unknown Tile"};
};

struct Tileset {
    TilesetName name{};
    int tile_size{};
    int tiles_per_row{};

    std::unordered_map<TileId, TileDef> tile_defs{};

public:
    const TileDef& get_tiledef_by_id(TileId id) const {
        auto it = tile_defs.find(id);
        if (it == tile_defs.end()) {
            throw TilesetError(std::format("Tile ID {} not found in tileset '{}'", id, name));
        }

        return it->second;
    }

    bool is_walkable(TileId id) const {
        return get_tiledef_by_id(id).walkable;
    }
};

} // namespace et_game

#endif
