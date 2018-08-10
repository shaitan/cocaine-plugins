#pragma once

#include <system_error>

namespace cocaine {
namespace vicodyn {

enum class vicodyn_errors {
    /// Upstream has been disconnected
    upstream_disconnected,

    /// Failed to retry enqueue
    failed_to_retry_enqueue,

    /// Failed to handle event for forward
    failed_to_handle_event_for_forward,

    /// Client has been disconnected
    client_disconnected,

    /// Failed to handle event for backward
    failed_to_handle_event_for_backward,
};

auto vicodyn_category() -> const std::error_category&;
auto make_error_code(vicodyn_errors err) -> std::error_code;
size_t constexpr vicodyn_category_id = 0x54FF;

} // namespace vicodyn
} // namespace cocaine

namespace std {

/// Extends the type trait `std::is_error_code_enum` to identify `vicodyn_errors` error codes.
template<>
struct is_error_code_enum<cocaine::vicodyn::vicodyn_errors> : public true_type {};

} // namespace std
