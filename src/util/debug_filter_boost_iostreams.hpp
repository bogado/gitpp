#ifndef DEBUG_FILTER_BOOST_IOSTREAMS_HPP_INCLUDED
#define DEBUG_FILTER_BOOST_IOSTREAMS_HPP_INCLUDED

#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/categories.hpp>

namespace debug {

    /// this models a debug filter that spits data into std::out.
    template <int c=1>
    struct debug_filter {
        using char_type = char;
        using category = boost::iostreams::input_filter_tag;

        template <class SOURCE>
        int get(SOURCE& source) {
            auto o = boost::iostreams::get(source);
            std::cout << std::dec <<  c << ":" << std::hex << std::showbase << std::setw(4) << std::internal << std::setfill('0') << o << "\n";
            return o;
        }
    };

}
#endif
