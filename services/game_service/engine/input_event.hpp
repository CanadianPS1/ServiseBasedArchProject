#ifndef ET_GAME_INPUT_EVENT_HPP
#define ET_GAME_INPUT_EVENT_HPP

#include <string>
#include <variant>
#include <nlohmann/json.hpp>

namespace et_game {

struct ConnectEvent {
    std::string user_id;
    std::string client_id;
    std::string mode;
};

struct DisconnectEvent {
    std::string client_id;
};

struct GameEvent {
    std::string client_id;
    nlohmann::json payload;
};

using InputEvent = std::variant<ConnectEvent, DisconnectEvent, GameEvent>;

} // namespace et_game

#endif
