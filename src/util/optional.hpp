#ifndef OPTIONAL_HPP_INCLUDED
#define OPTIONAL_HPP_INCLUDED

#if __has_include(<optional>)
#include <optional>
namespace git::opt {
    using std::optional;
}
#elif __has_include(<experimental/optional>)
#include <experimental/optional>
namespace git::opt {
    using std::experimental::optional;
}
#elif
#include <boost/optional.hpp>
namespace git::opt {
    using boost::optional
}
#endif

#endif
