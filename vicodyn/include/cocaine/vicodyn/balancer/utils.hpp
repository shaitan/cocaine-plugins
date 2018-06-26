#include <vector>

namespace cocaine {
namespace vicodyn {
namespace balancer {

auto uint64_rand() -> uint64_t;

template<class Iterator, class Predicate>
auto choose_random_if(Iterator begin, Iterator end, std::size_t size, Predicate p) -> Iterator {
    std::vector<Iterator> chosen;
    chosen.reserve(size);
    for(; begin != end; ++begin) {
        if(p(*begin)) {
            chosen.push_back(begin);
        }
    }
    if(chosen.empty()) {
        return end;
    }
    return chosen[uint64_rand() % chosen.size()];
}

} // namespace balancer
} // namespace vicodyn
} // namespace cocaine
