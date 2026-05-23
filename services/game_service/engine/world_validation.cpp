#include "world_loader.hpp"
#include "world_validation.hpp"

namespace et_game::detail {
void ValidationErrors::add(const std::string& error) {
    _errors.push_back(error);
}

bool ValidationErrors::empty() const {
    return _errors.empty();
}

std::size_t ValidationErrors::count() const {
    return _errors.size();
}

void ValidationErrors::throw_if_any_error() const {
    if(empty()) {
        return;
    }

    std::string formatted_errors = std::format(
        "Failed to validate world! Found {} error(s):\n",
        _errors.size()
    );
    
    for(const auto& error : _errors) {
        formatted_errors += " - " + error + "\n";
    }

    throw WorldLoadError(formatted_errors);
}

void validate_config(const World& world, ValidationErrors& errors) {
    const auto& start_id = world.config.starting_room_id;
    auto it = world.rooms.find(start_id);
    if(it == world.rooms.end()) {
        errors.add(std::format("Starting room with ID '{}' does not exist!", start_id));
        return;
    }

    const Room& starting_room = it->second;
    const Vec2& et_spawn_pos = world.config.et_spawn_pos;
    
    if(!starting_room.is_within_bounds(static_cast<int>(et_spawn_pos.x), static_cast<int>(et_spawn_pos.y))) {
        errors.add(std::format(
            "ET spawn position ({}, {}) is out of bounds for starting room '{}' (width: {}, height: {})!",
            et_spawn_pos.x, et_spawn_pos.y, start_id, starting_room.width, starting_room.height
        ));
    }
}

void validate_required_pieces(const World& world, ValidationErrors& errors) {
    for(const auto& required_piece_id : world.config.required_phone_pieces) {
        auto it = world.pickup_defs.find(required_piece_id);
        if(it == world.pickup_defs.end()) {
            errors.add(std::format("Required phone piece with ItemTypeId '{}' does not exist in pickup definitions!", required_piece_id));
            continue;
        }

        const PickupDef& pickup_def = it->second;
        if(pickup_def.kind != PickupDef::Kind::PhonePiece) {
            errors.add(std::format(
                "Required phone piece with ItemTypeId '{}' has invalid PickupType 'candy', expected 'PhonePiece'!", required_piece_id
            ));
            continue;
        }

        bool found = false;
        for(const auto& [_, room] : world.rooms) {
            for(const auto& spawn : room.pickup_spawns) {
                if(spawn.type == required_piece_id) {
                    found = true;
                    break;
                }
            }

            if(found) {
                break;
            }
        }

        if(!found) {
            errors.add(std::format(
                "Required phone piece with ItemTypeId '{}' is not in any room!",
                required_piece_id
            ));
        }
    }
}

void validate_exits(const World& world, ValidationErrors& errors) {
    for(const auto& [room_id, room] : world.rooms) {
        for(std::size_t i = 0; i < room.exits.size(); ++i) {
            const auto& exit = room.exits[i];
            auto it = world.rooms.find(exit.destination_room_id);
            if(it == world.rooms.end()) {
                errors.add(std::format(
                    "Exit {} in room '{}' leads to non-existent room '{}'",
                    i, room_id, exit.destination_room_id
                ));

                continue;
            }

            const Room& destination_room = it->second;
            const Vec2& spawn_pos = exit.spawn_pos;

            if(!destination_room.is_within_bounds(static_cast<int>(spawn_pos.x), static_cast<int>(spawn_pos.y))) {
                errors.add(std::format(
                    "Exit {} in room '{}' has spawn position ({}, {}) that is out of bounds for destination room '{}' (width: {}, height: {})!",
                    i, room_id, spawn_pos.x, spawn_pos.y, exit.destination_room_id, destination_room.width, destination_room.height
                ));
            }
        }
    }
}

void validate_pickup_spawns(const World& world, ValidationErrors& errors) {
    std::unordered_set<PickupId> seen_pickup_ids{};

    for(const auto& [room_id, room] : world.rooms) {
        for(std::size_t i = 0; i < room.pickup_spawns.size(); ++i) {
            const auto& spawn = room.pickup_spawns[i];

            if(!room.is_within_bounds(static_cast<int>(spawn.pos.x), static_cast<int>(spawn.pos.y))) {
                errors.add(std::format(
                    "Pickup spawn {} in room '{}' has position ({}, {}) that is out of bounds for the room (width: {}, height: {})!",
                    i, room_id, spawn.pos.x, spawn.pos.y, room.width, room.height
                ));
            }

            auto it = world.pickup_defs.find(spawn.type);
            if(it == world.pickup_defs.end()) {
                errors.add(std::format(
                    "Pickup spawn {} in room '{}' has undefined ItemTypeId '{}'",
                    i, room_id, spawn.type
                ));
            }

            if(seen_pickup_ids.contains(spawn.id)) {
                errors.add(std::format(
                    "Pickup spawn {} in room '{}' has duplicate PickupId '{}'",
                    i, room_id, spawn.id
                ));
            } 
            else {
                seen_pickup_ids.insert(spawn.id);
            }
        }
    }
}

void validate_room_reachability(const World& world, ValidationErrors& errors) {
    std::unordered_set<RoomId> visited{};
    std::queue<RoomId> to_visit{};

    to_visit.push(world.config.starting_room_id);

    while(!to_visit.empty()) {
        RoomId current_id = to_visit.front();
        to_visit.pop();

        if(visited.contains(current_id)) {
            continue;
        }

        visited.insert(current_id);

        const Room& current_room = world.get_room_by_id(current_id);
        for(const auto& exit : current_room.exits) {
            to_visit.push(exit.destination_room_id);
        }
    }

    for(const auto& [room_id, _] : world.rooms) {
        if(!visited.contains(room_id)) {
            errors.add(std::format("Room with ID '{}' is unreachable from the starting room!", room_id));
        }
    }
}
} // namespace et_game::detail
