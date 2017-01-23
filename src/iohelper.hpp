#ifndef IOHELPER_HPP_INCLUDED
#define IOHELPER_HPP_INCLUDED

#include <ios>

namespace git {

template <typename UNSIGNED_TYPE, typename STREAM>
auto read_netorder_at(STREAM& input, typename STREAM::pos_type position)
    -> typename std::enable_if<std::is_unsigned<UNSIGNED_TYPE>::value, UNSIGNED_TYPE>::type
{
    input.seekg(position, std::ios_base::beg);
    UNSIGNED_TYPE result = 0;
    uint8_t buffer[sizeof(UNSIGNED_TYPE)];
    input.read(reinterpret_cast<char*>(buffer), sizeof(buffer));

    return utils::from_buffer<UNSIGNED_TYPE>(buffer);
}

}

#endif
