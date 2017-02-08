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
    using const_reference = const value_type&;
    using const_pointer   = const value_type*;

    using difference_type = decltype(index_type{} - index_type{});
    using iterator_category = std::random_access_iterator_tag;

private:
    index_type index;
    const SOURCE* source;

public:
    indexed_iterator(index_type start, const SOURCE& ret) :
        index(start),
        source(&ret)
    {
    }

    indexed_iterator(const indexed_iterator&) = default;
    indexed_iterator(indexed_iterator&&) = default;
    indexed_iterator& operator=(const indexed_iterator&) = default;
    indexed_iterator& operator=(indexed_iterator&&) = default;

    auto operator-(const indexed_iterator& dest) {
        return index - dest.index;
    }

    auto operator+(index_type n) {
        return indexed_iterator{index + n, *source};
    }

    auto operator-(index_type n) {
        return indexed_iterator{index + n, *source};
    }

    auto operator+=(index_type n) {
        index += n;
    }

    auto operator-=(index_type n) {
        index -= n;
    }

    auto operator++() {
        index++;
        return *this;
    }

    auto operator++(int) {
        auto result{*this};
        index++;
        return result;
    }

    auto operator--() {
        index--;
        return *this;
    }

    auto operator--(int) {
        auto result{*this};
        index--;
        return result;
    }

    auto operator*() -> decltype((*source)[0]) {
        return (*source)[index];
    }

    auto operator[](index_type i) -> decltype((*source)[0]) {
        return (*source)[index + i];
    }

    bool operator==(const indexed_iterator& other) const {
        return source == other.source && index == other.index;
    }

    bool operator!=(const indexed_iterator& other) const {
        return !(*this == other);
    }

    bool operator<(const indexed_iterator& other) const {
        return index < other.index;
    }

    bool operator>(const indexed_iterator& other) const {
        return index > other.index;
    }

    bool operator>=(const indexed_iterator& other) const {
        return index >= other.index;
    }

    bool operator<=(const indexed_iterator& other) const {
        return index <= other.index;
    }

    index_type current_index() {
        return index;
    }
};

template <class SOURCE,
         class VALUE_TYPE,
         class ITERATOR = indexed_iterator<SOURCE> >
class index_iterable {
public:
    using value_type = VALUE_TYPE;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator = ITERATOR;

private:
    const SOURCE& getSelf() const {
         return *static_cast<const SOURCE*>(this);
    }

public:

    iterator begin() const {
        return iterator {0, getSelf()};
    }

    iterator end() const {
        return iterator {getSelf().size(), getSelf()};
    }

private:
};

}

#endif
