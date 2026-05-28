#include <chrono>
#include <thread>
#include <algorithm>

#include <spdlog/spdlog.h>

#include "engine/save/publisher_thread.hpp"

namespace et_game {

namespace {

constexpr int INITIAL_RETRY_SEC = 1;
constexpr int MAX_RETRY_SEC = 30;

void connect_with_retry(SavePublisher& publisher) {
    int retry_sec = INITIAL_RETRY_SEC;

    while(true) {
        try {
            publisher.connect();
            return;
        }
        catch(const SavePublisherError& e) {
            spdlog::warn("SavePublisher failed to connect! Error: {}, retrying in {}s", e.what(), retry_sec);
        }

        std::this_thread::sleep_for(std::chrono::seconds(retry_sec));
        retry_sec = std::min(retry_sec * 2, MAX_RETRY_SEC);
    }
}

} // anonymous namespace

void run_publisher_thread(SavePublisher &save_publisher, SaveEventQueue &save_queue) {
    spdlog::info("Publisher thread starting...");

    connect_with_retry(save_publisher);

    while(true) {
        auto event = save_queue.pop();
        if(!event.has_value()) {
            spdlog::info("Publisher thread received shutdown! Shutting down...");
            break;
        }

        bool has_sent = save_publisher.publish(event->body);
        if(!has_sent) {
            spdlog::warn("Failed to publish message (trigger='{}')! Reconnecting!", event->trigger);
            save_publisher.disconnect();

            connect_with_retry(save_publisher);

            has_sent = save_publisher.publish(event->body);
            if(!has_sent) {
                spdlog::error("Publish failed after reconnect (trigger='{}')! Dropping event!", event->trigger);
            }
        }
        else {
            spdlog::debug("Published save event (trigger='{}')!", event->trigger);
        }
    }

    save_publisher.disconnect();
    spdlog::info("Publisher thread exited!");
}

} // namespace et_game

