#ifndef DEFER_HPP
#define DEFER_HPP

#include <utility>

namespace utils {

template<class Fn>
struct Defer final {
    [[nodiscard]] explicit Defer(Fn &&func): _func(std::forward<Fn>(func)) {}

    ~Defer() {
        _func();
    }

private:
    Fn _func;
};

}

#endif //DEFER_HPP
