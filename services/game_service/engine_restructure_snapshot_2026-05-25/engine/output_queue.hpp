#ifndef ET_GAME_OUTPUT_QUEUE_HPP
#define ET_GAME_OUTPUT_QUEUE_HPP

#include <queue>
#include <mutex>
#include <string>
#include <optional>
#include <condition_variable>

namespace et_game {

class OutputQueue {
public:
    void push(std::string msg) {
        {
            std::lock_guard lock(_mutex);
            _queue.push(std::move(msg));
        }
        _cv.notify_one();
    }

    std::optional<std::string> wait_and_pop() {
        std::unique_lock lock(_mutex);
        _cv.wait(lock, [this]() { return !_queue.empty() || _shutdown; });

        if(_shutdown && _queue.empty()) {
            return std::nullopt;
        }

        std::string msg = std::move(_queue.front());
        _queue.pop();

        return msg;
    }

    void shutdown() {
        {
            std::lock_guard lock(_mutex);
            _shutdown = true;
        }
        _cv.notify_all();
    }

private:
    std::mutex _mutex{};
    std::condition_variable _cv{};
    std::queue<std::string> _queue{};

    bool _shutdown = false;
};

} // namespace et_game

#endif
