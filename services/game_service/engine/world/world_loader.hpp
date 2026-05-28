#ifndef ET_GAME_WORLD_LOADER_HPP
#define ET_GAME_WORLD_LOADER_HPP

#include "models/types.hpp"
#include "models/world.hpp"
#include "models/tileset.hpp"

namespace et_game {

struct WorldLoadError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

World load_world(const std::string& world_file_path, const std::string& tileset_dir_path);

} // namespace et_game

#endif
