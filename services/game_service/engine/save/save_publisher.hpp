#ifndef ET_GAME_SAVE_PUBLISHER_HPP
#define ET_GAME_SAVE_PUBLISHER_HPP

#include <string>
#include <cstdint>
#include <stdexcept>

#include <SimpleAmqpClient/SimpleAmqpClient.h>

namespace et_game {

struct SavePublisherError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class SavePublisher {
public:
    SavePublisher(
        std::string host,
        std::uint16_t port,
        std::string username,
        std::string password,
        std::string queue_name
    );

    ~SavePublisher() { disconnect(); }

    SavePublisher(const SavePublisher&) = delete;
    SavePublisher& operator=(const SavePublisher&) = delete;

    void connect();
    void disconnect();
    bool publish(const std::string& msg_body);

    bool is_connected() const { return _channel != nullptr; }

private:
    std::string _host{};
    std::uint16_t _port{};
    std::string _username{};
    std::string _password{};
    std::string _queue_name{};

    AmqpClient::Channel::ptr_t _channel{nullptr};
};

} // namespace et_game

#endif
