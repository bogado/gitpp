#ifndef IOHELPER_HPP_INCLUDED
#define IOHELPER_HPP_INCLUDED

#include <ios>

namespace git {

template <typename UNSIGNED_TYPE, typename SOURCE,
         typename STREAM = typename std::remove_reference<decltype(*(static_cast<SOURCE*>(nullptr)->stream()))>::type>
             auto read_netorder_at(const SOURCE& source, typename STREAM::pos_type position)
             -> typename std::enable_if<std::is_unsigned<UNSIGNED_TYPE>::value, UNSIGNED_TYPE>::type
{
    auto input = source.substream(position);
    uint8_t buffer[sizeof(UNSIGNED_TYPE)];
    input->read(reinterpret_cast<char*>(buffer), sizeof(buffer));

    return utils::from_buffer<UNSIGNED_TYPE>(buffer);
}

namespace {

inline char hex_digit (unsigned val) {
    char v = val & 0x0f;
    if (v < 10) {
        return '0' + v;
    } else {
        return 'a' + (v - 10);
    }
}

inline auto hex_value(char d) {
    if (d >= 'a' && d <= 'f') { return d - 'a' + 10; }
    if (d >= 'A' && d <= 'F') { return d - 'A' + 10; }
    if (d >= '0' && d <= '9') { return d - '0'; }
    return 0;
}

}

inline auto read_object_name_from(std::istream& input) {
    char buffer[OBJECT_NAME_SIZE];
    input.read(buffer, OBJECT_NAME_SIZE);

    std::string result(OBJECT_NAME_SIZE * 2, ' ');
    auto it = result.begin();
    for (char v: buffer) {
        *it = hex_digit(v >> 4);
        it++;
        *it = hex_digit(v & 0x0f);
        it++;
    }

    return result;
}

}

#endif
