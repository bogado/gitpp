#ifndef FILESYSTEM_HPP_INCLUDED
#define FILESYSTEM_HPP_INCLUDED

#if __has_include(<filesystem>)
#include <filesystem>

namespace git::fs {
    using namespace std::filesystem;
    using std::ifstream;
}
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace git::fs {
    using namespace std::experimental::filesystem;
    using std::ifstream;
}
#else
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace git::fs {
    using namespace boost::filesystem;
}
#endif

#endif
