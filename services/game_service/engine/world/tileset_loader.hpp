#ifndef ET_GAME_TILESET_LOADER_HPP
#define ET_GAME_TILESET_LOADER_HPP

#include <string>

#include "models/tileset.hpp"

namespace et_game {

Tileset load_tileset(
    const std::string& tileset_path,
    const std::string& tileset_name
);

} // namespace et_game

#endif
