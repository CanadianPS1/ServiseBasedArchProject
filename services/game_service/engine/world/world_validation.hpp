#ifndef ET_GAME_WORLD_VALIDATION_HPP
#define ET_GAME_WORLD_VALIDATION_HPP

#include <queue>
#include <string>
#include <vector>
#include <format>
#include <unordered_set>
#include <unordered_map>

#include "models/types.hpp"
#include "models/world.hpp"
#include "models/tileset.hpp"

namespace et_game::detail {

class ValidationErrors {
public:
    void add(const std::string& error);
    bool empty() const;
    std::size_t count() const;

    void throw_if_any_error() const;

private:
    std::vector<std::string> _errors;
};

void validate_exits(const World& world, ValidationErrors& errors);
void validate_config(const World& world, ValidationErrors& errors);
void validate_tilesets(const World& world, ValidationErrors& errors);
void validate_pickup_spawns(const World& world, ValidationErrors& errors);
void validate_required_pieces(const World& world, ValidationErrors& errors);
void validate_room_reachability(const World& world, ValidationErrors& errors);
} // namespace et_game::detail

#endif
