#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include "game_loop.hpp"

namespace et_game {

static constexpr double TICK_HZ = 30.0;
static constexpr double TICK_INTERVAL_SEC = 1.0 / TICK_HZ;

void run_game_loop() {
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
            accumulated_time -= TICK_INTERVAL_SEC;
            spdlog::debug("Tick {} - frame_time: {:.3f}s, accumulated_time: {:.3f}s", tick_count, frame_time, accumulated_time);
            tick_count++;
        }

        // NOTE: Sleep thread for 1ms avoid CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

} // namespace et_game
