#ifndef ET_GAME_SERVICE_WORLD_HPP
#define ET_GAME_SERVICE_WORLD_HPP

#include "types.hpp"

#include <vector>
#include <format>
#include <stdexcept>
#include <unordered_map>

namespace et_game {
struct Exit {
    Vec2 spawn_pos;
    // NOTE: This is the direction the exit is on from the perspective of the room it is in. E.g.: an exit on the north wall would have Direction::North, even if it leads south to the next room.
    Direction edge_direction;
    RoomId destination_room_id;
};

struct PickupSpawn {
    TilePos pos;
    PickupId id;
    ItemTypeId type;
};

struct PickupDef {
    enum class Kind {
        PhonePiece,
        Candy
    };

    Kind kind = Kind::PhonePiece;
    std::string display_name = "Unknown Pickup";
};

struct Room {
    RoomId id;

    int width = 0;
    int height = 0;
    TilesetName tileset_name;

    std::vector<Exit> exits;
    std::vector<TileId> tiles;
    std::vector<PickupSpawn> pickup_spawns;

    inline TileId tile_at(int x, int y) const {
        return tiles[y * width + x];
    }

    inline bool is_within_bounds(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }
};

struct WorldConfig {
    float starting_energy = 100.0f;
    float energy_drain_rate = 0.5f;
    float energy_per_candy = 25.0f;

    Vec2 et_spawn_pos;
    RoomId starting_room_id;
    std::vector<ItemTypeId> required_phone_pieces;
};

struct World {
    int schema_version = 0;
    WorldConfig config;

    std::unordered_map<RoomId, Room> rooms;
    std::unordered_map<ItemTypeId, PickupDef> pickup_defs;

    inline const Room& get_room_by_id(const RoomId& room_id) const {
        auto it = rooms.find(room_id);
        if(it == rooms.end()) {
            throw std::out_of_range(std::format("Room with ID '{}' not found", room_id));
        }

        return it->second;
    }

    inline const PickupDef& get_pickup_def_by_id(const ItemTypeId& item_type_id) const {
        auto it = pickup_defs.find(item_type_id);
        if(it == pickup_defs.end()) {
            throw std::out_of_range(std::format("PickupDef with ItemTypeId '{}' not found", item_type_id));
        }

        return it->second;
    }
};

} // namespace et_game
#endif
