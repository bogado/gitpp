#ifndef OBJECT_DESCRIPTOR_HPP_INCLUDED
#define OBJECT_DESCRIPTOR_HPP_INCLUDED

#include <iostream>
#include <string>

namespace git {

class object_descriptor_base {
public:
    virtual ~object_descriptor_base() = default;

    /// Returns the name used by git.
    virtual const std::string& get_name() const = 0;

    /// Checks the validity of this object.
    virtual operator bool() const = 0;

    /// Gets an inputs stream that reads from this object.
    virtual std::istream& get_stream() = 0;
};

}

#endif
