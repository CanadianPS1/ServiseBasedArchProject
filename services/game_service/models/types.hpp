#ifndef ET_GAME_SERVICE_TYPES_HPP
#define ET_GAME_SERVICE_TYPES_HPP

#include <string>

namespace et_game {

using RoomId = std::string;
using TileId = unsigned char;
using PickupId = std::string;
using ItemTypeId = std::string;
using TilesetName = std::string;

enum class Direction {
    North,
    South,
    East,
    West
};

struct TilePos {
    int x = 0;
    int y = 0;

    friend inline bool operator==(const TilePos& lhs, const TilePos& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend inline bool operator!=(const TilePos& lhs, const TilePos& rhs) {
        return !(lhs == rhs);
    }
};

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    friend inline bool operator==(const Vec2& lhs, const Vec2& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend inline bool operator!=(const Vec2& lhs, const Vec2& rhs) {
        return !(lhs == rhs);
    }
};

}; // namespace et_game

#endif
