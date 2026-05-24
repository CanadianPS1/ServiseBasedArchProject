#include <chrono>
#include <thread>
#include <format>
#include <optional>

#include <httplib.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "visitor.hpp"
#include "game_loop.hpp"
#include "game_state.hpp"
#include "save_client.hpp"
#include "input_event.hpp"
#include "message_builders.hpp"
#include "game_event_handlers.hpp"

namespace et_game {

static constexpr double TICK_HZ = 30.0;
static constexpr double TICK_INTERVAL_SEC = 1.0 / TICK_HZ;
static constexpr float MOVE_SPEED = 4.0f; // tiles per second

void handle_connect(
    const ConnectEvent& connect_event,
    const World& world,
    std::optional<GameState>& game_state,
    OutputQueue& output_queue
) {
    spdlog::info("Connect: user='{}', mode='{}'", connect_event.user_id, connect_event.mode);

    try {
        if(connect_event.mode == "continue") {
            auto save_json = fetch_save_json(connect_event.user_id);
            game_state = save_json.has_value() 
                ? GameState(connect_event.user_id, *save_json)
                : GameState(connect_event.user_id, world);
        }
        else {
            game_state = GameState(connect_event.user_id, world);
        }

        output_queue.push(build_room_loaded(world, *game_state));
    }
    catch(const std::exception& e) {
        spdlog::error("Failed to start game session! {}", e.what());
        game_state.reset();
    }
}

void handle_disconnect(const DisconnectEvent& disconnect_event, std::optional<GameState>& game_state) {
    spdlog::info("Disconnect: client='{}!'", disconnect_event.client_id);
    game_state.reset();
}

void handle_game_event(const GameEvent& game_event, std::optional<GameState>& game_state) {
    if(!game_state.has_value()) {
        return;
    }

    dispatch_game_event(*game_state, game_event.payload);
}

void apply_movement_event(GameState& game_state, double dt) {
    if(!game_state.is_playing()) { return; }

    Vec2& player_pos = game_state.player.local_pos;
    if(game_state.input_state.move_north) { player_pos.y -= MOVE_SPEED * dt; } 
    if(game_state.input_state.move_south) { player_pos.y += MOVE_SPEED * dt; }
    if(game_state.input_state.move_east)  { player_pos.x += MOVE_SPEED * dt; }
    if(game_state.input_state.move_west)  { player_pos.x -= MOVE_SPEED * dt; }

}

void tick(GameState& game_state, double dt, OutputQueue& output_queue) {
    apply_movement_event(game_state, dt);
    output_queue.push(build_state_update(game_state));
}

void run_game_loop(
    const World& world,
    InputQueue& input_queue,
    OutputQueue& output_queue
) {
    using Clock = std::chrono::steady_clock;
    using std::chrono::duration;

    spdlog::info("Starting game loop with tick rate of {} Hz (tick interval: {}ms)", TICK_HZ, TICK_INTERVAL_SEC * 1000);

    auto last_tick_time = Clock::now();
    double accumulated_time = 0.0;

    std::optional<GameState> game_state{};

    while(true) {
        auto curr_tick_time = Clock::now();
        double frame_time = duration<double>(curr_tick_time - last_tick_time).count();
        last_tick_time = curr_tick_time;

        if(frame_time > 0.25) {
            spdlog::warn("Frame time of {:.3f}s is too high, clamping to 0.25s to avoid spiral of death!", frame_time);
            frame_time = 0.25;
        }

        accumulated_time += frame_time;

        while(accumulated_time >= TICK_INTERVAL_SEC) {
            while(auto event = input_queue.try_pop()) {
                std::visit(Overloaded {
                    [&](const ConnectEvent& connect_event) { handle_connect(connect_event, world, game_state, output_queue); },
                    [&](const DisconnectEvent& disconnect_event) { handle_disconnect(disconnect_event, game_state); },
                    [&](const GameEvent& game_event) { handle_game_event(game_event, game_state); }
                    }, *event
                );
            }

            if(game_state.has_value()) {
                tick(*game_state, TICK_INTERVAL_SEC, output_queue);
            }

            accumulated_time -= TICK_INTERVAL_SEC;
        }

        // NOTE: Sleep thread for 1ms avoid CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace et_game
