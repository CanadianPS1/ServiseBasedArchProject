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

inline Direction opposite_direction(Direction direction) {
    switch(direction) {
        case Direction::North: return Direction::South;
        case Direction::South: return Direction::North;
        case Direction::East:  return Direction::West;
        case Direction::West:  return Direction::East;
    }

    // NOTE: This should never happen, but we'll return North as a fallback
    return Direction::North;
}

inline const char *direction_to_string(Direction direction) {
    switch(direction) {
        case Direction::North: return "N";
        case Direction::South: return "S";
        case Direction::East:  return "E";
        case Direction::West:  return "W";
    }

    return "?";
}

struct TilePos {
    int x = 0;
    int y = 0;

    friend bool operator==(const TilePos& lhs, const TilePos& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend bool operator!=(const TilePos& lhs, const TilePos& rhs) {
        return !(lhs == rhs);
    }
};

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    friend bool operator==(const Vec2& lhs, const Vec2& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend bool operator!=(const Vec2& lhs, const Vec2& rhs) {
        return !(lhs == rhs);
    }
};

} // namespace et_game

#endif
