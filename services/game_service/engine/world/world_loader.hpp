#ifndef ET_GAME_WORLD_LOADER_HPP
#define ET_GAME_WORLD_LOADER_HPP

#include "models/types.hpp"
#include "models/world.hpp"
#include "models/tileset.hpp"

namespace et_game {

struct WorldLoadError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// NOTE: load_world() and validate_tilesets() form a loose contract and NEED to be called successively. 
// This is for simplicity and to avoid unnecessary coupling between the world loading and tileset loading processes.
// In the future, if we want to decouple these processes, we can consider returning a more complex result from load_world()
// that includes any necessary information for validation, or we can have load_world() take in the tilesets as an argument and 
// perform validation internally.
World load_world(const std::string& world_file_path);

void validate_tilesets(
    const World& world,
    const std::unordered_map<TilesetName, Tileset>& tilesets
);

} // namespace et_game
#endif
