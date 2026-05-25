#ifndef ET_GAME_INPUT_QUEUE_HPP
#define ET_GAME_INPUT_QUEUE_HPP

#include <queue>
#include <mutex>
#include <string>
#include <optional>

#include "engine/events/input_event.hpp"

namespace et_game {

class InputQueue {
public:
    void push(InputEvent msg) {
        std::lock_guard lock(_mutex);
        _queue.push(std::move(msg));
    }

    std::optional<InputEvent> try_pop() {
        std::lock_guard lock(_mutex);
        if(_queue.empty()) {
            return std::nullopt;
        }

        InputEvent msg = std::move(_queue.front());
        _queue.pop();

        return msg;
    }

private:
    std::mutex _mutex{};
    std::queue<InputEvent> _queue{};
};

} // namespace et_game

#endif
