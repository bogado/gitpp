#ifndef OBJECT_DESCRIPTOR_HPP_INCLUDED
#define OBJECT_DESCRIPTOR_HPP_INCLUDED

#include <iostream>
#include <string>

namespace git {

class object_descriptor_base {
public:
    virtual ~object_descriptor_base() = default;
    virtual const std::string& get_name() const = 0;
};

}

#endif
