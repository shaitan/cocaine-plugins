#pragma once

#include <vector>

namespace cocaine {
namespace vicodyn {
namespace balancer {

auto uint64_rand() -> uint64_t;

template<typename Iterator>
auto choose_random(Iterator begin, Iterator end) -> Iterator {
    static_assert(std::is_same<typename Iterator::iterator_category, std::random_access_iterator_tag>::value,
            "Function works very slow for containers with no random access iterator");
    auto size = std::distance(begin, end);
    if (!size) {
        return end;
    }
    std::advance(begin, uint64_rand() % size);
    return begin;
}

} // namespace balancer
} // namespace vicodyn
} // namespace cocaine
