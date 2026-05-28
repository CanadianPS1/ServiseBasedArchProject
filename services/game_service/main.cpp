#include <string>
#include <thread>
#include <cstdlib>
#include <exception>

#include <spdlog/spdlog.h>

#include "models/tileset.hpp"

#include "engine/session/game_loop.hpp"
#include "engine/world/world_loader.hpp"
#include "engine/save/save_publisher.hpp"
#include "engine/save/publisher_thread.hpp"
#include "engine/network/sender_thread.hpp"
#include "engine/util/concurrent_queue.hpp"
#include "engine/network/websocket_server.hpp"

namespace detail {

std::string env_var_or(const char *env_var_name, const std::string& default_fallback) {
    const char *value = std::getenv(env_var_name);
    return value ? std::string(value) : default_fallback;
}

std::uint16_t env_var_or(const char *env_var_name, const std::uint16_t default_fallback) {
    char *value = std::getenv(env_var_name);
    return value ? static_cast<std::uint16_t>(std::atoi(value)) : default_fallback;
}

} // namespace detail

int main() {
    const std::string world_path = detail::env_var_or("WORLD_JSON_PATH", "../shared/static/world.json");
    const std::string tileset_dir_path = detail::env_var_or("TILESET_DIR_PATH", "../shared/tileset_metadata");

    const std::string   rabbitmq_host = detail::env_var_or("RABBITMQ_HOST", "0.0.0.0");
    const std::uint16_t rabbitmq_port = detail::env_var_or("RABBITMQ_PORT", 5672);
    const std::string   rabbitmq_user = detail::env_var_or("RABBITMQ_USER", "guest");
    const std::string   rabbitmq_pass = detail::env_var_or("RABBITMQ_PASSWORD", "guest");
    const char *SAVES_QUEUE_NAME = "saves";

    et_game::World world{};
    try {
        world = et_game::load_world(world_path, tileset_dir_path);
    }
    catch(const et_game::WorldLoadError& e) {
        spdlog::error("Failed to load world: {}", e.what());
        return EXIT_FAILURE;
    }
    catch(const et_game::TilesetError& e) {
        spdlog::error("Failed to load tileset: {}", e.what());
        return EXIT_FAILURE;
    }
    catch(const std::exception& e) {
        spdlog::error("An unexpected error occurred: {}", e.what());
        return EXIT_FAILURE;
    }

    et_game::InputQueue input_queue{};
    et_game::OutputQueue output_queue{};
    et_game::SaveEventQueue save_queue{};

    et_game::WebsocketServer websocket_server(8001, input_queue);

    et_game::SavePublisher save_publisher(
        rabbitmq_host, rabbitmq_port,
        rabbitmq_user, rabbitmq_pass,
        SAVES_QUEUE_NAME
    );

    std::thread websocket_thread([&websocket_server]() {
        websocket_server.run();
    });

    spdlog::info("Websocket server thread running!");

    std::thread sender_thread([&websocket_server, &output_queue]() {
        et_game::run_sender_thread(websocket_server, output_queue);
    });

    std::thread publisher_thread([&save_publisher, &save_queue]() {
        et_game::run_publisher_thread(save_publisher, save_queue);
    });

    spdlog::info("Sender thread running!");
    spdlog::info("Starting game loop...");
    
    et_game::run_game_loop(world, input_queue, output_queue, save_queue);

    output_queue.shutdown();
    save_queue.shutdown();

    publisher_thread.join();
    sender_thread.join();
    websocket_thread.join();

    return EXIT_SUCCESS;
}
