#ifndef BIG_INTEGER_HPP_INCLUDED
#define BIG_INTEGER_HPP_INCLUDED

#include <cstdint>
#include <limits>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include <exception>
#include <vector>
#include <array>
#include <typeinfo>
#include <sstream>

#include <boost/io/ios_state.hpp>

namespace git {

template <typename INT,
         unsigned reserved_bits = 0,
         bool big_endian = false,
         class storage = std::vector<INT>,
         unsigned value_bits = sizeof(INT)*8 - 1,
         typename = typename std::enable_if<
            std::is_integral<INT>::value &&
            std::is_same<typename storage::value_type, INT>::value
         >::type>
class big_unsigned_base {
    using iterator = typename storage::iterator;
    using const_iterator = typename storage::const_iterator;
    using reverse_iterator = typename storage::reverse_iterator;
    using const_reverse_iterator = typename storage::const_reverse_iterator;
    using buffer = std::array<char, sizeof(INT)>;

    storage values;

    static constexpr unsigned mask = 1 << value_bits;
    static constexpr unsigned value_mask = mask - 1;

    static constexpr unsigned non_reserved_bits = (value_bits - reserved_bits);
    static constexpr unsigned non_reserved_mask = (1 << non_reserved_bits) - 1;
    static constexpr unsigned reserved_mask = ((1 << reserved_bits) - 1) << non_reserved_bits;
    static constexpr unsigned base = mask;

public:
    using value_type = INT;
    using big_unsigned_type = big_unsigned_base;

    static constexpr unsigned bits_per_unit = value_bits;

    unsigned get_reserved_bits() const {
        if (values.empty()) {
            return 0;
        }
        return (values.front() & reserved_mask) >> non_reserved_bits;
    }

    void set_reserved_bits(unsigned value) {
        value <<= non_reserved_bits;
        value &= reserved_mask;

        if (values.empty()) {
            values.push_back(value);
            return;
        }

        value |= (values.front() & ~(reserved_mask));
        values.front() = value;
    }

    void clear() {
        values.clear();
    }

    iterator begin() {
        return values.begin();
    }

    iterator end() {
        return values.end();
    }

    const_iterator begin() const {
        return values.cbegin();
    }

    const_iterator end() const {
        return values.cend();
    }

    reverse_iterator rbegin() {
        return values.rbegin();
    }

    reverse_iterator rend() {
        return values.rend();
    }

    const_reverse_iterator crbegin() const {
        return values.crbegin();
    }

    const_reverse_iterator crend() const {
        return values.crend();
    }

    std::size_t size() const {
        return values.size();
    }

    unsigned bit_size() const {
        auto s = size();
        if (s == 0) {
            return 0;
        }
        return s*bits_per_unit - reserved_bits;
    }

/* comparison */
    bool operator==(const big_unsigned_base& other) const {
        if (other.values.size() != values.size()) {
            return false;
        }
        return std::equal(std::begin(values), std::end(values), std::begin(other.values));
    }


/* encoding values -> big_unsigned */
private:
    constexpr void insert(value_type value) {
        if (big_endian) {
            if (!values.empty()) {
                values.back() |= mask;
            }
            values.insert(std::end(values), value);
        } else {
            if (!values.empty()) {
                value |= mask;
            }
            values.insert(std::begin(values), value);
        }
    }

public:
    template <typename T,
              typename ENABLED = typename std::enable_if<
                std::is_integral<T>::value>::type
            >
    big_unsigned_base& operator=(T value) {
        clear();

        insert(value & non_reserved_mask);
        value >>= non_reserved_bits;
        while (value > 0) {
            if (!big_endian) { value--; }

            insert(value & value_mask);
            value >>= value_bits;
        }
        return *this;
    }

    template <typename T,
              typename ENABLED = typename std::enable_if<
                std::is_integral<T>::value>::type
            >
    explicit big_unsigned_base(T value) :
        big_unsigned_base()
    {
        (*this) = value;
    }

    big_unsigned_base() = default;
    big_unsigned_base(const big_unsigned_base&) = default;
    big_unsigned_base(big_unsigned_base&&) = default;
    big_unsigned_base& operator=(const big_unsigned_base&) = default;
    big_unsigned_base& operator=(big_unsigned_base&&) = default;

/* decoding big_unsigned => value */
private:
    template <typename T>
    static constexpr T update_result(T old_result, value_type value, unsigned used_bits) {
        auto result = old_result;

        if (big_endian) {
            result = result + (static_cast<T>(value) << used_bits);
        } else {
            result = ((result + 1) << bits_per_unit) + value;
        }

        if (result > std::numeric_limits<T>::max()) {
            throw std::overflow_error((
                        std::string("Not enough bit in conversion (")
                        + std::to_string(result)
                        + " > " +  std::to_string(std::numeric_limits<T>::max())
                        + " " + typeid(T).name() + " )").c_str());
        }
        return static_cast<T>(result);
    }

public:
    template <typename T>
    T convert() const {
        auto v = begin();

        if (v == end()) {
            return 0;
        }

        T result { static_cast<T>(non_reserved_mask & *v)};
        if (is_terminator(*v)) {
            return result;
        }

        unsigned used_bits = non_reserved_bits;

        while (++v != end()) {

            result = update_result(result, *v & value_mask, used_bits);

            used_bits += bits_per_unit;

            if (is_terminator(*v)) {
                break;
            }

        }

        return result;
    }

    /* I/O routines */
    template <typename OStream>
    friend OStream& operator<<(OStream& out, const big_unsigned_base& me) {
        auto flags = out.flags();
        auto value = me.convert<uintmax_t>();
        out << "0x" << std::hex << value;
        out.flags(flags);
        return out;
    }

    template <class IStream>
    void binread(IStream& in) {
        value_type val;
        buffer buff;
        clear();
        while(in.read(buff.data(), buff.size())) {
            val = unsigned_from_buffer(buff);
            values.push_back(val);

            if (is_terminator(val)) {
                break;
            }
        }
    }

    template <class OStream>
    void binwrite(OStream& out) {
        for (const auto& val: values) {
            buffer buff;
            unsigned_to_buffer(val, buff);
            out.write(buff.data(), buff.size());
        }
    }

    template <typename stream>
    void dump(stream& out) const {
        boost::io::ios_flags_saver save(out);
        out << "<" << (big_endian?"big_endian":"little_endian") << ", "
            << reserved_bits << "+" << non_reserved_bits << ">[ ";
        out << std::setfill('0');
        std::hex(out);
        for (const auto& val : values) {
            out << std::setw(sizeof(val)*2) << unsigned(val) << " ";
        }
        out << "]";
    }

    std::string dump() const {
        std::stringstream out;
        dump(out);
        return out.str();
    }

private:

    static value_type unsigned_from_buffer(
            const buffer& buf) {
        value_type result = 0;
        for (size_t i = 0; i < sizeof(value_type); i++) {
            result += buf[i] * (1 << i * 8);
        }
        return result;
    }

    static void  unsigned_to_buffer(value_type val, buffer& result) {
        static constexpr value_type byte_mask = 0xFF;
        for (auto& v: result) {
            using v_type = typename std::remove_reference<decltype(v)>::type;
            v = static_cast<v_type>(val & byte_mask);
            val /= (1 << sizeof(v_type)*8);
        }
    }

    static constexpr bool is_terminator(value_type v) {
        return (v & mask) == 0;
    }
};

static constexpr auto GIT_TYPE_ENCODE_BITS = 3;
using big_unsigned            = big_unsigned_base<std::uint8_t>;
using big_unsigned_with_type  = big_unsigned_base<std::uint8_t, GIT_TYPE_ENCODE_BITS, true>;
using big_unsigned_big_endian = big_unsigned_base<std::uint8_t, 0, true>;

}

#endif
