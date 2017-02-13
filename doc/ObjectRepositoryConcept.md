# Concept: Object repository

An object that models an object repository models a collection of git objects. Each object is indexed by an [object description](ObjectDescription).

## Requirements

### inner types

* ``value_type`` : type that models an [object description](ObjectDescription)
* ``reference``  : reference to the ``value_type``
* ``pointer``    : pointer to the ``value_type``

### Operations

* ``repository[ IDENTIFICATION ]``     : returns a [object description](ObjectDescription) for the object that corresponds to the given identification. If the object is not found an empty description will be returned. This operation can be costly (up to O(N)) if the ID is a string, it must be O(1) if the IDENTIFICATION is already a description. It may rquire I/O to finish.
* ``begin()`` / ``end()``              : iterate throught all the objects inside of this repository.

