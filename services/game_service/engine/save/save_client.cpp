#include <format>
#include <cstdlib>

#include <httplib.h>
#include <spdlog/spdlog.h>

#include "engine/save/save_client.hpp"

namespace et_game {

namespace {

const std::string AUTH_HOST = []{
    const char *env_auth_host = std::getenv("AUTH_HOST");
    return env_auth_host ? std::string(env_auth_host) : "auth_service";
}();

const int AUTH_PORT = []{
    const char *env_auth_port = std::getenv("AUTH_PORT");
    return env_auth_port ? std::atoi(env_auth_port) : 8000;
}();

constexpr int CONNECTION_TIMEOUT_SEC = 2;
constexpr int READ_TIMEOUT_SEC = 2;

} // anonymous namespace

std::optional<Json> fetch_save_json(const std::string& user_id) {
    spdlog::debug("Fetching save for user '{}' from {}:{}", user_id, AUTH_HOST, AUTH_PORT);

    httplib::Client client(AUTH_HOST, AUTH_PORT);
    client.set_connection_timeout(CONNECTION_TIMEOUT_SEC);
    client.set_read_timeout(READ_TIMEOUT_SEC);

    auto res = client.Get(("/get_game_state/" + user_id).c_str());
    if(!res) {
        throw SaveClientError(std::format(
            "Failed to fetch save for '{}'! Network error: {}",
            user_id, httplib::to_string(res.error())
        ));
    }

    if(res->status == 404) {
        spdlog::debug("No save found for user '{}'", user_id);
        return std::nullopt;
    }

    if(res->status != 200) {
        throw SaveClientError(std::format(
            "Failed to fetch save for '{}'! Unexpected HTTP status {}",
            user_id, res->status
        ));
    }

    try {
        auto json = Json::parse(res->body);
        spdlog::debug("Save loaded for user '{}'!", user_id);

        return json;
    }
    catch(const Json::parse_error& e) {
        throw SaveClientError(std::format(
            "Failed to parse save for '{}': '{}'!", 
            user_id, e.what()
        ));
    }
}

} // namespace et_game
