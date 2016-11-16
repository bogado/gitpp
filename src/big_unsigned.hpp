#ifndef BIG_INTEGER_HPP_INCLUDED
#define BIG_INTEGER_HPP_INCLUDED

#include <cstdint>
#include <limits>
#include <iostream>
#include <type_traits>
#include <exception>
#include <vector>

namespace git {

template <typename INT,
         unsigned reserved_bits = 0,
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

    static constexpr unsigned bits_per_unit = value_bits;

    unsigned get_reserved_bits() const {
        if (values.empty()) {
            return 0;
        }
        return (values.front() & reserved_mask) >> non_reserved_bits;
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
            buffer buff = unsigned_to_buffer(val);
            out.write(buff.data(), buff.size());
        }
    }

    template <typename stream>
    void dump(stream& out) {
        auto state = out.rdstate();
        out << "[ " << std::hex;
        for (auto val : values) {
            out << unsigned(val) << " ";
        }
        out << "]";
        out.setstate(state);
    }
private:
    template <typename T, unsigned bits>
    void insert_one_step(T& value) {
        static value_type bits_base = 1 << bits;
        static value_type bits_mask = bits_base - 1;

        auto& new_value = *values.insert(std::begin(values), value & bits_mask);
        value /= bits_base;
        if (value > 0) {
            new_value |= mask;
        }
    }

public:
    template <typename T,
              typename = typename std::enable_if<
                std::is_integral<T>::value>::type
            >
    big_unsigned_base& operator=(T value) {
        clear();
        insert_one_step<T, non_reserved_bits>(value);
        while (value > 0) {
            insert_one_step<T, bits_per_unit>(value);
        }
        return *this;
    }

    template <typename T,
              typename std::enable_if<
                std::is_integral<T>::value>::type
            >
    big_unsigned_base(T value) :
        big_unsigned_base()
    {
        (*this) = value;
    }

    big_unsigned_base() = default;
    big_unsigned_base(const big_unsigned_base&) = default;
    big_unsigned_base(big_unsigned_base&&) = default;
    big_unsigned_base& operator=(const big_unsigned_base&) = default;
    big_unsigned_base& operator=(big_unsigned_base&&) = default;

    template <typename T>
    T convert() const {
        static constexpr auto BITS = sizeof(T)*8;
        auto v = begin();

        if (v == end()) {
            return 0;
        }

        T result = non_reserved_mask & *v;
        if (is_terminator(*v)) {
            return result;
        }
        unsigned used_bits = bits_used(result);

        while (++v != end()) {
            used_bits += bits_per_unit;
            if (used_bits > BITS) {
                throw std::overflow_error((
                            std::string("Not enough bit in conversion (")
                            + std::to_string(used_bits)
                            + " > " + std::to_string(BITS)
                            + " " + typeid(T).name() + " )").c_str());
            }
            result = (result << bits_per_unit) + (*v & value_mask);
            if (is_terminator(*v)) {
                break;
            }
        }
        return result;
    }

    template <typename Stream>
    friend Stream& operator<<(Stream& out, const big_unsigned_base& me) {
        auto flags = out.flags();

        static unsigned hex_digit_mask = 0xf;
        static unsigned hex_digit_bits = 4;

        bool start=true;

        out << "0x" << std::hex;
        auto it = me.begin();
        if (it == me.end()) {
            out << "0";
            return out;
        } else {
            unsigned current = non_reserved_mask & *it;
            unsigned current_bits = non_reserved_bits;
            unsigned total_bits = me.size()*bits_per_unit - reserved_bits;

            while (total_bits > 0) {
                unsigned consume_bits = total_bits % hex_digit_bits;
                consume_bits = consume_bits == 0 ? hex_digit_bits : consume_bits;

                unsigned move = current_bits - consume_bits;
                unsigned v = current >> move;

                if (! start || v != 0) {
                    start = false;
                    out << v;
                }

                total_bits -= consume_bits;
                current_bits -= consume_bits;
                current &= (1 << current_bits) - 1;

                if (current_bits < hex_digit_bits) {
                    if (it == me.end()) {
                        break;
                    }
                    ++it;

                    current = (current << bits_per_unit) + (*it & value_mask);
                    current_bits += bits_per_unit;
                }
            }
        }

        out.flags(flags);
        return out;
    }

private:

    constexpr static bool is_terminator(value_type val) {
        return (val & mask) == 0;
    }

    static value_type unsigned_from_buffer(const buffer& buf) {
        static constexpr unsigned char byte_base = 0x100;
        value_type result = 0;
        for (char v: buf) {
            result = result * byte_base + v;
        }
        return result;
    }

    static buffer&& unsigned_to_buffer(value_type val) {
        static constexpr unsigned char byte_mask = 0xFF;
        buffer result;
        for (char& v: result) {
            v = val % byte_mask;
            val /= 256;
        }
        return std::move(result);
    }

    static constexpr value_type is_end(value_type v) {
        return v & mask;
    }

    template <typename T>
    static constexpr unsigned bits_used(T val) {
        unsigned t = 0;
        while (val > 0) {
            val /= 2;
            t++;
        }
        return t;
    }
};

using big_unsigned = big_unsigned_base<std::uint8_t>;

}

#endif
