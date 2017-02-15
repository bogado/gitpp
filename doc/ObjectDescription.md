# Object Description

Class that represents a generic object description that can be turned into an [stream](http://en.cppreference.com/w/cpp/io/basic_istream) that reads the object content. Object description implementation have an intimate related with the repository that created them. The Reader streams can be used to generate a git object.

## Requirements

Object descriptions should be trivially constructed and efficiently copy constructed. The description can be empty, in which case it will be trivially convertible to false.

Has to inherit from the interface ``description_base``.

### Operations

* ``static_cast<bool>(description)`` : True if this describe an existing object.
* ``get_name()``                     : Returns the name of this object. Returns an empty string if and only if this descirption is empty.
* ``get_stream()``                   : Returns the ``istream`` of this object.
