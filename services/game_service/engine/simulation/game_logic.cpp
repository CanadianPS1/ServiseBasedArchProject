#include <spdlog/spdlog.h>

#include "engine/simulation/game_logic.hpp"
#include "engine/protocol/message_builders.hpp"

namespace et_game {

static constexpr float MOVE_SPEED = 4.0f; // tiles per second

void apply_movement_event(GameState& game_state, double dt) {
    if(!game_state.is_playing()) { 
        return; 
    }

    Vec2& player_pos = game_state.player.local_pos;
    if(game_state.input_state.move_north) { player_pos.y -= MOVE_SPEED * dt; } 
    if(game_state.input_state.move_south) { player_pos.y += MOVE_SPEED * dt; }
    if(game_state.input_state.move_east)  { player_pos.x += MOVE_SPEED * dt; }
    if(game_state.input_state.move_west)  { player_pos.x -= MOVE_SPEED * dt; }
}

bool handle_exit_transitions(
    GameState& game_state,
    const World& world,
    OutputQueue& output_queue
) {
    if(!game_state.is_playing()) { 
        return false; 
    }
    
    const Room& room = game_state.current_room(world);
    Vec2& player_pos = game_state.player.local_pos;
    
    Direction crossed{};
    bool any_crossed{false};

    if(player_pos.y < 0.0f) { crossed = Direction::North; any_crossed = true; }
    else if(player_pos.y >= static_cast<float>(room.height)) { crossed = Direction::South; any_crossed = true; }
    else if(player_pos.x < 0.0f) { crossed = Direction::West; any_crossed = true; }
    else if(player_pos.x >= static_cast<float>(room.width)) { crossed = Direction::East; any_crossed = true; }

    if(!any_crossed) { 
        return false; 
    }

    const Exit* exit = room.find_exit_on_side(crossed);
    if(!exit) { 
        if(player_pos.x < 0) { player_pos.x = 0; }
        if(player_pos.y < 0) { player_pos.y = 0; }
        if(player_pos.x >= room.width)  { player_pos.x = room.width - 0.0001f; }
        if(player_pos.y >= room.height) { player_pos.y = room.width - 0.0001f; }

        return false; 
    }

    game_state.player.current_room_id = exit->destination_room_id;
    game_state.player.local_pos = exit->spawn_pos;
    game_state.player.facing = exit->edge_direction;

    output_queue.push(build_room_loaded(world, game_state, opposite_direction(crossed)));
    return true;
}

/*
This is what I was talking about earlier with spdlog with warnings that resolve after I rebuild. Is cmake hiding warnings or something the second time?



Here's the current code, if it's all good give me some testing inputs and things to test to make sure everything is working correctly. 



Also, do we really need to clamp the value if there's no exit? Wouldn't tile collision detect that? I mean, we can keep it, but I was just wondering if my thinking was correct or not. I guess we need it for now since we have no tile collision detection at the moment, but if that's the case then it should've been mentioned that this clamping is temporary
*/

void handle_pickup_collection(GameState& game_state, const World& world) {
    if(!game_state.is_playing()) { return; }

    const Room& room = game_state.current_room(world);
    const int player_tile_x = static_cast<int>(game_state.player.local_pos.x);
    const int player_tile_y = static_cast<int>(game_state.player.local_pos.y);

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

        spdlog::info("Collected pickup '{}' of type '{}'", pickup_spawn.id, pickup_spawn.type);
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

void check_win_lose(GameState& game_state, const World& world) {
    if(game_state.has_won(world)) {
        game_state.status = GameStatus::Won;
        spdlog::info("User '{}' has won the game!", game_state.user_id);
        return;
    }
    else if(game_state.energy <= 0.0f) {
        game_state.status = GameStatus::Lost;
        spdlog::info("User '{}' has lost the game!", game_state.user_id);
        return;
    }
}

} // namespace et_game

