#ifndef INDEXED_ITERATOR_HPP_INCLUDED
#define INDEXED_ITERATOR_HPP_INCLUDED

namespace git {

template <class SOURCE>
class indexed_iterator {
public:
    using index_type = typename SOURCE::index_type;
    using value_type = typename SOURCE::value_type;
    using reference = value_type&;
    using pointer   = value_type*;

private:
    index_type index;
    SOURCE& source;

public:
    indexed_iterator(index_type start, SOURCE& ret) :
        index(start),
        source(ret) {}

    auto operator++() {
        index++;
        return this;
    }

    auto operator++(int) {
        auto result = *this;
        index++;
        return result;
    }

    auto operator--() {
        index--;
        return this;
    }

    auto operator--(int) {
        auto result = *this;
        index--;
        return result;
    }

    auto operator*() const {
        return source[index];
    }

    bool operator==(const indexed_iterator& other) const {
        return &source == &other.source && index == other.index;
    }

    bool operator!=(const indexed_iterator& other) const {
        return !(*this == other);
    }

    index_type current_index() {
        return index;
    }
};

template <class SOURCE,
         class VALUE_TYPE,
         class ITERATOR = indexed_iterator<SOURCE>>
class index_iterable {
    SOURCE& base;
public:
    index_iterable(SOURCE& b) :
        base(b)
    {}

    using value_type = VALUE_TYPE;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator = ITERATOR;

    iterator begin() const {
        return iterator {0, base};
    }

    iterator end() const {
        return iterator {base.size(), base};
    }
};

}

#endif
