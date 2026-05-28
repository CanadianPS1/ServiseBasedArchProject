#include <format>

#include <spdlog/spdlog.h>

#include "engine/save/save_publisher.hpp"
#include "SimpleAmqpClient/BasicMessage.h"

namespace et_game {

SavePublisher::SavePublisher(
    std::string host,
    std::uint16_t port,
    std::string username,
    std::string password,
    std::string queue_name
):
    _host(std::move(host)),
    _port(port),
    _username(std::move(username)),
    _password(std::move(password)),
    _queue_name(std::move(queue_name))
{}

void SavePublisher::connect() {
    if(_channel) {
        return;
    }

    try {
        _channel = AmqpClient::Channel::Create(_host, _port, _username, _password);

        _channel->DeclareQueue(
            _queue_name,
            /* passive     */ false,
            /* durable     */ true,
            /* exclusive   */ false,
            /* auto_delete */ false
        );
    }
    catch(const std::exception& e) {
        _channel.reset();
        
        throw SavePublisherError(std::format(
            "AMQP error while connecting to {}:{}: {}",
            _host, _port, e.what()
        ));
    }

    spdlog::info("SavePublisher connected to {}:{} (queue='{}')!", _host, _port, _queue_name);
}

void SavePublisher::disconnect() {
    if(!_channel) {
        return;
    }

    _channel.reset();
    spdlog::info("SavePublisher disconnected!");
}

bool SavePublisher::publish(const std::string& msg_body) {
    if(!_channel) {
        spdlog::warn("Failed to publish message! SavePublisher not connected!");
        return false;
    }

    spdlog::info("Publishing message: {} to amqp server {}:{}!", msg_body, _host, _port);

    try {
        auto msg = AmqpClient::BasicMessage::Create(msg_body);
        msg->ContentType("application/json");
        msg->DeliveryMode(AmqpClient::BasicMessage::dm_persistent);

        _channel->BasicPublish(
            "",
            _queue_name,
            msg
        );

        return true;
    }
    catch(const std::exception& e) {
        spdlog::warn("Failed to publish message! Unexpected error: {}", e.what());
        _channel.reset();

        return false;
    }
}

} // namespace et_game

