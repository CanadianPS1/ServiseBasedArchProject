#include <chrono>
#include <thread>
#include <format>
#include <optional>

#include <httplib.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "visitor.hpp"
#include "game_loop.hpp"
#include "input_event.hpp"
#include "../models/game_state.hpp"

namespace et_game {

namespace {

constexpr double TICK_HZ = 30.0;
constexpr double TICK_INTERVAL_SEC = 1.0 / TICK_HZ;

constexpr float MOVE_SPEED = 4.0f; // tiles per second

constexpr const char *AUTH_HOST = "auth_service";
constexpr std::uint16_t AUTH_PORT = 8000;

std::optional<GameState> get_save_for_user(const std::string& user_id) {
    httplib::Client client(AUTH_HOST, AUTH_PORT);
    client.set_connection_timeout(2);
    client.set_read_timeout(2);
}

} // anonymous namespace 

void run_game_loop(InputQueue& input_queue) {
    using Clock = std::chrono::steady_clock;
    using std::chrono::duration;

    spdlog::info("Starting game loop with tick rate of {} Hz (tick interval: {}ms)", TICK_HZ, TICK_INTERVAL_SEC * 1000);

    auto last_tick_time = Clock::now();
    double accumulated_time = 0.0;
    std::uint64_t tick_count = 0;

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
            tick_count++;
            accumulated_time -= TICK_INTERVAL_SEC;
        }

        // NOTE: Sleep thread for 1ms avoid CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace et_game
