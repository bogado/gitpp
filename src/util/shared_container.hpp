#include <memory>
#include <utility>
#include <type_traits>
#include <iterator>

namespace git {

namespace {

template <typename CONTAINER, typename ENABLE = void>
struct container_operations {
    using container_type = CONTAINER;

    // Element_t is the shared_ptr::element_type,
    // this is different for arrays and proper stl containers.
    // element_type<int[4]> = int*;
    //    while
    // element_type<std::array<int, 4>> = std::array<int, 4>;
    using element_type = container_type;

    using shared_ptr = std::shared_ptr<element_type>;
    using unique_ptr = std::unique_ptr<element_type>;


    static size_t max_size_of(const CONTAINER& c) {
        return c.max_size();
    }

    static auto make_shared() {
        return std::make_shared<container_type>();
    }

    static auto make_unique() {
        return std::make_shared<container_type>();
    }
};

template <typename ARRAY>
struct container_operations<ARRAY, typename std::enable_if<std::is_array<ARRAY>::value>::type > {
    // Element_t is the shared_ptr::element_type,
    using element_type = typename std::remove_extent<ARRAY>::type;
    static constexpr size_t SIZE = sizeof(ARRAY)/sizeof(element_type);

    using container_type = element_type[SIZE];


    using shared_ptr = std::shared_ptr<container_type>;
    using unique_ptr = std::unique_ptr<container_type>;

    static size_t max_size_of(const container_type&) {
        return SIZE;
    }

    static auto make_shared() {
        return shared_ptr(new element_type[SIZE], [](element_type* d) {
            delete[] d;
        });
    }

    static auto make_unique() {
        return unique_ptr(new element_type[SIZE], [](element_type* d) {
            delete[] d;
        });
    }
};

template <typename CONTAINER>
struct shared_container_traits : public container_operations<CONTAINER> {
    using container_type = CONTAINER;

    using size_type = decltype(std::size(std::declval<container_type>()));

    using iterator = decltype(std::begin(std::declval<container_type&>()));
    using const_iterator = decltype(std::cbegin(std::declval<container_type&>()));
    using iterator_traits = std::iterator_traits<iterator>;

    using value_type = typename iterator_traits::value_type;
    using reference = typename iterator_traits::reference;
    using pointer = typename iterator_traits::pointer;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = typename iterator_traits::iterator_category;
};

}

/*** A shared_ptr view of a container.
 *
 * This class is an adapter to a shared_ptr reference of a container.
 * Instances will hold a reference and each iterator will also hold
 * references. This allows us to keep the original alive as long as
 * some iterator to it exists.
 */
template <class CONTAINER, class CONTAINER_TRAITS = shared_container_traits<CONTAINER>>
class shared_container {
    using container_traits = CONTAINER_TRAITS;
    using container_type = CONTAINER;

    using shared_ptr = typename container_traits::shared_ptr;
    using unique_ptr= typename container_traits::unique_ptr;
    using element_type = typename container_traits::element_type;

    using original_iterator = typename container_traits::iterator;
    using original_const_iterator = typename container_traits::const_iterator;
public:
    using value_type = typename container_traits::value_type;
    using reference = typename container_traits::reference;
    using const_reference = const reference;
    using difference_type = typename container_traits::difference_type;
    using size_type = typename container_traits::size_type;

private:
    template <class original_iterator>
    class base_iterator {
        using original_iterator_traits = typename std::iterator_traits<original_iterator>;
    public:
        using difference_type = typename original_iterator_traits::difference_type;
        using value_type = typename original_iterator_traits::value_type;
        using pointer = typename original_iterator_traits::pointer;
        using reference = typename original_iterator_traits::reference;
        using iterator_category = std::input_iterator_tag;

        auto operator*() {
            return *iterator;
        }

        auto operator++() {
            ++iterator;
            return *this;
        }

        base_iterator(std::shared_ptr<container_type> container_ptr, original_iterator iterator_) :
            original(container_ptr),
            iterator(iterator_)
        {}

        std::shared_ptr<container_type> original;
        original_iterator iterator;
    };

public:
    using iterator = base_iterator<original_iterator>;
    using const_iterator = base_iterator<original_const_iterator>;

    shared_container() :
        original{container_traits::make_shared()}
    {}

    explicit shared_container(const shared_ptr& shared) :
        original{shared}
    {}

    explicit shared_container(unique_ptr&& unique) :
        original{std::move(unique)}
    {}

    shared_container(container_type&& container) :
        original{std::make_shared<container_type>(std::move(container))}
    {}

    shared_container(const container_type& container) :
        shared_container{}
    {
        std::copy(std::begin(container), std::end(container), std::begin(get()));
    }

    shared_container(iterator begin_, iterator end_) :
        shared_container{}
    {
        std::copy(begin_, end_, std::begin(get()));
    }

    auto begin() {
        return iterator(original, std::begin(get()));
    }

    auto end() {
        return iterator(original, std::end(get()));
    }

    auto cbegin() {
        return const_iterator(original, original->begin());
    }

    auto cend() {
        return const_iterator(original, original->begin());
    }

    auto begin() const {
        return cbegin();
    }

    auto end() const {
        return cend();
    }

    const auto& get() const {
        if constexpr (std::is_same_v<element_type, container_type>) {
            return *original.get();
        } else {
            // This is fine since I'm the one that built this shared_ptr.
            return *(reinterpret_cast<container_type*>(original.get()));
        }
    }

    auto& get() {
        if constexpr (std::is_same_v<element_type, container_type>) {
            return *original.get();
        } else {
            // This is fine since I'm the one that built this shared_ptr.
            return *(reinterpret_cast<container_type*>(original.get()));
        }
    }

    const auto& operator*() const { return get(); }

    auto& operator*() { return get(); }

    auto operator->() {
        return original.get();
    }


    auto operator==(const shared_container& b) const {
        return *original == *b.original;
    }

    auto size() const {
        return std::size(get());
    }

    auto max_size() const {
        return container_traits::max_size_of(get());
    }

    auto empty() const {
        return original->empty();
    }

    void swap(shared_container& b) {
        std::swap(original, b.original);
    }

private:
    shared_ptr original;
};

template <typename CONTAINER_T, typename...  ARG_TYPES>
shared_container<CONTAINER_T> make_shared_container(ARG_TYPES&&... args)
{
    using element_type = typename shared_container_traits<CONTAINER_T>::element_type;

    return shared_container<CONTAINER_T>{std::make_shared<element_type>(std::forward<ARG_TYPES>(args)...)};
}

template <typename CONTAINER_T, typename ELEMENT_TYPE>
shared_container<CONTAINER_T> make_shared_container(std::initializer_list<ELEMENT_TYPE> initializer_list)
{
    using element_type = typename shared_container_traits<CONTAINER_T>::element_type;

    return shared_container<CONTAINER_T>{std::make_shared<element_type>(initializer_list)};
}

}
