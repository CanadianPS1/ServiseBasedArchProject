#include <chrono>
#include <format>

#include <nlohmann/json.hpp>

#include "engine/save/save_event_builder.hpp"

namespace et_game {

using Json = nlohmann::json;

static std::string current_utc() {
    auto now = std::chrono::floor<std::chrono::seconds>(
        std::chrono::system_clock::now()
    );

    return std::format("{:%FT%TZ}", now);
}

SaveEvent build_save_event(const GameState& game_state, const std::string& trigger) {
    Json save_event{};
    save_event["userId"]  = game_state.user_id;
    save_event["savedAt"] = current_utc();
    save_event["trigger"] = trigger;
    save_event["state"]   = game_state.to_json();

    return SaveEvent {
        .body = save_event.dump(),
        .trigger = trigger
    };
}

} // namespace et_game
