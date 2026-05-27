#ifndef ET_GAME_SAVE_CLIENT_HPP
#define ET_GAME_SAVE_CLIENT_HPP

#include <string>
#include <optional>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace et_game {

using Json = nlohmann::json;

struct SaveClientError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::optional<Json> fetch_save_json(const std::string& user_id);

} // namespace et_game

#endif
