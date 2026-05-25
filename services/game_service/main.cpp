#include <string>
#include <thread>
#include <exception>
#include <spdlog/spdlog.h>

#include "engine/session/game_loop.hpp"
#include "engine/network/input_queue.hpp"
#include "engine/network/output_queue.hpp"
#include "engine/world/world_loader.hpp"
#include "engine/network/sender_thread.hpp"
#include "engine/network/websocket_server.hpp"

int main() {
    /* TODO: Restore once done testing
    const char *world_env_path = std::getenv("WORLD_JSON_PATH");
    const std::string world_path = world_env_path
        ? std::string(world_env_path)
        : "../shared/static/world.json";
    */

    const std::string world_path = "../shared/static/test_world.json";
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting ET Game Service...");
    spdlog::info("Loading world from '{}'...", world_path);

    et_game::World world{};
    try {
        world = et_game::load_world(world_path);
    }
    catch(const et_game::WorldLoadError& e) {
        spdlog::error("Failed to load world: {}", e.what());
        return EXIT_FAILURE;
    }
    catch(const std::exception& e) {
        spdlog::error("An unexpected error occurred: {}", e.what());
        return EXIT_FAILURE;
    }

    et_game::InputQueue input_queue{};
    et_game::OutputQueue output_queue{};
    et_game::WebsocketServer websocket_server(8001, input_queue);

    std::thread websocket_thread([&websocket_server]() {
        websocket_server.run();
    });

    spdlog::info("Websocket server thread running!");

    std::thread sender_thread([&websocket_server, &output_queue]() {
        et_game::run_sender_thread(websocket_server, output_queue);
    });

    spdlog::info("Sender thread running!");
    spdlog::info("Starting game loop...");
    
    et_game::run_game_loop(world, input_queue, output_queue);

    output_queue.shutdown();
    sender_thread.join();
    websocket_thread.join();

    return EXIT_SUCCESS;
}
