#include <random>

namespace cocaine {
namespace vicodyn {
namespace balancer {

auto uint64_rand() -> uint64_t {
    // We use keywords "static" and "thread_local", because this function is called many times
    // from different threads, and creation of entities below is very expensive.
    static thread_local std::mt19937 gen(std::random_device{}());
    // TODO: Use "constexpr" instead "thread_local" when std::uniform_int_distribution::operator()(...) become a const.
    static thread_local std::uniform_int_distribution<uint64_t> distribution(0, std::numeric_limits<uint64_t>::max());
    return distribution(gen);
}

} // namespace balancer
} // namespace vicodyn
} // namespace cocaine
