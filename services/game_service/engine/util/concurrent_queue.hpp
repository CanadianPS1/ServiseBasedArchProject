#ifndef ET_GAME_CONCURRENT_QUEUE_HPP
#define ET_GAME_CONCURRENT_QUEUE_HPP

#include <queue>
#include <mutex>
#include <string>
#include <utility>
#include <optional>
#include <condition_variable>

#include "engine/save/save_event.hpp"
#include "engine/events/input_event.hpp"


namespace et_game {

template<typename T>
class ConcurrentQueue {
public:
    ConcurrentQueue() = default;

    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

    void push(T value) {
        {
            std::lock_guard lock(_mutex);
            _queue.push(std::move(value));
        }

        _cv.notify_one();
    }

    std::optional<T> try_pop() {
        std::lock_guard lock(_mutex);
        if(_queue.empty()) {
            return std::nullopt;
        }

        T value = std::move(_queue.front());
        _queue.pop();

        return value;
    }

    std::optional<T> pop() {
        std::unique_lock lock(_mutex);
        _cv.wait(lock, [this]{ return !_queue.empty() || _shutdown == true; });

        if(_queue.empty()) {
            return std::nullopt;
        }

        T value = std::move(_queue.front());
        _queue.pop();

        return value;
    }

    void shutdown() {
        {
            std::lock_guard lock(_mutex);
            _shutdown = true;
        }

        _cv.notify_all();
    }

private:
    std::queue<T> _queue{};
    std::mutex _mutex{};
    std::condition_variable _cv{};
    bool _shutdown{false};
};

using InputQueue = ConcurrentQueue<InputEvent>;
using OutputQueue = ConcurrentQueue<std::string>;
using SaveEventQueue = ConcurrentQueue<SaveEvent>;

} // namespace et_game

#endif
