#include <spdlog/spdlog.h>

#include "engine/simulation/game_logic.hpp"
#include "engine/protocol/message_builders.hpp"
#include "engine/save/save_event.hpp"
#include "engine/save/save_event_builder.hpp"

namespace et_game {

namespace {

constexpr float MOVE_SPEED = 4.0f; // tiles per second

bool is_tile_walkable(
    const World& world,
    const Room& room,
    int tile_x,
    int tile_y
) {
    if(tile_x < 0 || tile_y < 0) {
        return false;
    }

    if(tile_x >= room.width || tile_y >= room.height) {
        return false;
    }

    const TileId tile_id = room.tile_at(tile_x, tile_y);
    const Tileset& tileset = world.get_tileset_by_name(room.tileset_name);

    return tileset.is_walkable(tile_id);
}

} // anonymous namespace

void apply_movement_event(
    GameState& game_state,
    const World& world,
    double dt
) {
    if(!game_state.is_playing()) { 
        return; 
    }

    const Room& room = game_state.current_room(world);
    Vec2& player_pos = game_state.player.local_pos;

    float dx = 0.0f;
    float dy = 0.0f;
    if(game_state.input_state.move_north) { dy -= MOVE_SPEED  * dt; } 
    if(game_state.input_state.move_south) { dy += MOVE_SPEED  * dt; }
    if(game_state.input_state.move_east)  { dx  += MOVE_SPEED * dt; }
    if(game_state.input_state.move_west)  { dx  -= MOVE_SPEED * dt; }

    {
    float new_x = player_pos.x + dx;
        int new_tile_x  = static_cast<int>(new_x);
        int curr_tile_y = static_cast<int>(player_pos.y);

        bool is_out_of_room = new_tile_x < 0 || new_tile_x >= room.width;
        if(is_out_of_room || is_tile_walkable(world, room, new_tile_x, curr_tile_y)) {
            player_pos.x = new_x;
        }
    }

    {
        float new_y = player_pos.y + dy;
        int curr_tile_x = static_cast<int>(player_pos.x);
        int new_tile_y  = static_cast<int>(new_y);

        bool is_out_of_room = new_tile_y < 0 || new_tile_y >= room.height;
        if(is_out_of_room || is_tile_walkable(world, room, curr_tile_x, new_tile_y)) {
            player_pos.y = new_y;
        }
    }
}

bool handle_exit_transitions(
    GameState& game_state,
    const World& world,
    OutputQueue& output_queue,
    SaveEventQueue& save_queue
) {
    if(!game_state.is_playing()) { 
        return false; 
    }
    
    const Room& room = game_state.current_room(world);
    Vec2& player_pos = game_state.player.local_pos;
    
    Direction crossed{};
    bool any_crossed{false};

    if(player_pos.y < 0.0f)                                  { crossed = Direction::North; any_crossed = true; }
    else if(player_pos.y >= static_cast<float>(room.height)) { crossed = Direction::South; any_crossed = true; }
    else if(player_pos.x < 0.0f)                             { crossed = Direction::West; any_crossed = true; }
    else if(player_pos.x >= static_cast<float>(room.width))  { crossed = Direction::East; any_crossed = true; }

    if(!any_crossed) { 
        return false; 
    }

    const Exit* exit = room.find_exit_on_side(crossed);
    if(!exit) { 
        // TODO: Remove once tile collision is implemented
        if(player_pos.x < 0) { player_pos.x = 0; }
        if(player_pos.y < 0) { player_pos.y = 0; }
        if(player_pos.x >= room.width)  { player_pos.x = room.width  - 0.0001f; }
        if(player_pos.y >= room.height) { player_pos.y = room.height - 0.0001f; }

        return false; 
    }

    game_state.player.current_room_id = exit->destination_room_id;
    game_state.player.local_pos = exit->spawn_pos;
    game_state.player.facing = exit->edge_direction;

    output_queue.push(build_room_loaded(world, game_state, opposite_direction(crossed)));
    save_queue.push(build_save_event(game_state, "exit_transition"));
    
    return true;
}

void handle_pickup_collection(
    GameState& game_state,
    const World& world,
    SaveEventQueue& save_queue
) {
    if(!game_state.is_playing()) { return; }

    const Room& room = game_state.current_room(world);
    const int player_tile_x = static_cast<int>(game_state.player.local_pos.x);
    const int player_tile_y = static_cast<int>(game_state.player.local_pos.y);

    bool collected_anything{};
    for(const auto& pickup_spawn : room.pickup_spawns) {
        if(!game_state.is_pickup_active(pickup_spawn.id)) { continue; }
        if(static_cast<int>(pickup_spawn.pos.x) != player_tile_x) { continue; }
        if(static_cast<int>(pickup_spawn.pos.y) != player_tile_y) { continue; }

        game_state.collected_pickups.insert(pickup_spawn.id);

        const PickupDef& pickup_def = world.get_pickup_def_by_id(pickup_spawn.type);
        if(pickup_def.kind == PickupDef::Kind::PhonePiece) {
            game_state.collected_phone_pieces.push_back(pickup_spawn.type);
        }
        else if(pickup_def.kind == PickupDef::Kind::Candy) {
            game_state.candy_count++;
        }

        collected_anything = true;
        spdlog::info("Collected pickup '{}' of type '{}'", pickup_spawn.id, pickup_spawn.type);
    }

    if(collected_anything) {
        save_queue.push(build_save_event(game_state, "pickup"));
    }
}

void drain_energy(GameState& game_state, const World& world, double dt) {
    if(!game_state.is_playing()) {
        return; 
    }

    game_state.energy -= world.config.energy_drain_rate * static_cast<float>(dt);
    if(game_state.energy < 0.0f) {
        game_state.energy = 0.0f;
    }
}

void check_win_lose(
    GameState& game_state,
    const World& world,
    SaveEventQueue& save_queue
) {
    if(game_state.has_won(world)) {
        game_state.status = GameStatus::Won;
        spdlog::info("User '{}' has won the game!", game_state.user_id);
    
        save_queue.push(build_save_event(game_state, "win"));
    }
    else if(game_state.energy <= 0.0f) {
        game_state.status = GameStatus::Lost;
        spdlog::info("User '{}' has lost the game!", game_state.user_id);
    
        save_queue.push(build_save_event(game_state, "lose"));
    }
}

} // namespace et_game

