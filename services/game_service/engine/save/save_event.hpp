#ifndef ET_GAME_SAVE_EVENT_HPP
#define ET_GAME_SAVE_EVENT_HPP

#include <string>

namespace et_game {

struct SaveEvent {
    std::string body;
    std::string trigger;  
};

} // namespace et_game

#endif
