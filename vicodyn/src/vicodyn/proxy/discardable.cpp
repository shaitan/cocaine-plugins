#include "cocaine/vicodyn/proxy/discardable.hpp"

namespace cocaine {
namespace vicodyn {

discardable_dispatch_t::discardable_dispatch_t(const std::string& name)
    : dispatch<app_tag_t>(name) {
}

auto discardable_dispatch_t::discard(const std::error_code& ec) -> void {
    if(discarder_) {
        discarder_(ec);
    }
}

auto discardable_dispatch_t::on_discard(discarder_t discarder) -> void {
    discarder_ = std::move(discarder);
}

} // namespace vicodyn
} // namespace cocaine
