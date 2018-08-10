#pragma once

#include "cocaine/vicodyn/proxy/protocol.hpp"

#include <cocaine/hpack/header.hpp>

namespace cocaine {
namespace vicodyn {

class catcher_t {
    using on_catch_t = std::function<void(const std::system_error& e)>;
    const on_catch_t on_catch_;

public:
    catcher_t(on_catch_t on_catch);
    catcher_t(const catcher_t& catcher) = default;
    catcher_t(catcher_t&& catcher);

    template<typename F, typename Event, typename... Args>
    auto operator()(F fn, Event, const hpack::headers_t& headers, Args&&... args) -> void {
        try {
            fn(headers, std::forward<Args>(args)...);
        }
        catch(const std::system_error& e) {
            on_catch_(e);
        }
    }
};

class before_t {
    using callback_t = std::function<void()>;
    const callback_t callback_;

public:
    before_t(callback_t callback);
    before_t(const before_t& before) = default;
    before_t(before_t&& before);

    template<typename F, typename Event, typename... Args>
    auto operator()(F fn, Event, const hpack::headers_t& headers, Args&&... args) -> void {
        callback_();
        fn(headers, std::forward<Args>(args)...);
    }
};

class pre_check_error_t {
    using condition_t = std::function<bool(const std::error_code& ec, const std::string& msg)>;
    const condition_t condition_;

public:
    pre_check_error_t(condition_t condition);
    pre_check_error_t(const pre_check_error_t& pre_check) = default;
    pre_check_error_t(pre_check_error_t&& pre_check);

    template<typename F, typename Event>
    typename std::enable_if<std::is_same<Event, protocol_t::error>::value, void>::type
    operator()(F fn, Event, const hpack::headers_t& headers, std::error_code&& ec, std::string&& msg) {
        if (condition_(ec, msg)) {
            fn(headers, std::move(ec), std::move(msg));
        }
    }
};

} // namespace vicodyn
} // namespace cocaine
