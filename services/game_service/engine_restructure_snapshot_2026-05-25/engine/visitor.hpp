#ifndef ET_GAME_VISITOR_HPP
#define ET_GAME_VISITOR_HPP

namespace et_game {

template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace et_game

#endif
