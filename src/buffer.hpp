#ifndef BUFFER_HPP_INCLUDED
#define BUFFER_HPP_INCLUDED

#include <array>

namespace git {
namespace utils {

template <class INT, class BUFFER=std::array<std::uint8_t, sizeof(INT)> >
INT from_buffer(const BUFFER& buffer) {
    INT result = 0;
    for (auto value : buffer) {
        result <<= 8;
        result += value;
    }
    return result;
}

template <typename INT, class BUFFER=std::array<std::uint8_t, sizeof(INT)> >
BUFFER to_buffer(INT value) {
    static constexpr auto MASK = (2 << sizeof(typename BUFFER::value_t) * 8) - 1;
    BUFFER result;

    for (int i = size_of(value) - 1; i >= 0; i--) {
        result[i] = value % MASK;
        value >>= 8;
    }
    return result;
}

}
}

#endif
