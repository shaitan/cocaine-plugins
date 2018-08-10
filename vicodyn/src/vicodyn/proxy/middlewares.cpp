#include "cocaine/vicodyn/proxy/middlewares.hpp"

namespace cocaine {
namespace vicodyn {

catcher_t::catcher_t(on_catch_t on_catch)
    : on_catch_(std::move(on_catch)) {
}

catcher_t::catcher_t(catcher_t&& catcher)
    : on_catch_(std::move(catcher.on_catch_)) {
}

before_t::before_t(callback_t callback)
    : callback_(std::move(callback)) {
}

before_t::before_t(before_t&& before)
    : callback_(std::move(before.callback_)) {
}

pre_check_error_t::pre_check_error_t(condition_t condition)
    : condition_(std::move(condition)) {

}

pre_check_error_t::pre_check_error_t(pre_check_error_t&& pre_check)
    : condition_(std::move(pre_check.condition_)) {
}

} // namespace vicodyn
} // namespace cocaine
