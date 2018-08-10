#pragma once

#include "cocaine/vicodyn/forwards.hpp"
#include "cocaine/vicodyn/proxy/buffer.hpp"
#include "cocaine/vicodyn/proxy/discardable.hpp"
#include "cocaine/vicodyn/proxy/middlewares.hpp"
#include "cocaine/vicodyn/proxy/safe_stream.hpp"
#include "cocaine/vicodyn/request_context.hpp"

namespace cocaine {
namespace vicodyn {

class vicodyn_dispatch_t;

class endpoint_t  {
    using clock_t = peers_t::clock_t;

    std::shared_ptr<peer_t> peer_;
    safe_stream_t forward_stream_;
    std::unique_ptr<logging::logger_t> logger_;
    clock_t::time_point start_time_;
    bool error_sent_ = false;
    buffer_t buffer_;
    std::mutex mutex_;

    discardable_dispatch_t backward_dispatch_;
    std::atomic_bool backward_started_{false};

    const std::string app_name_;
    peers_t& peers_;
    const api::vicodyn::balancer_ptr balancer_;
    const std::shared_ptr<request_context_t> request_context_;
    logging::logger_t& base_logger_;
    std::weak_ptr<vicodyn_dispatch_t> life_cycle_;

public:
    endpoint_t(const std::string& dispatch_name, const std::string& app_name, std::size_t buffer_capacity,
            api::vicodyn::balancer_ptr balancer, peers_t& peers, std::shared_ptr<request_context_t> request_context,
            logging::logger_t& logger);

    auto enqueue(const hpack::headers_t& headers, std::string&& event, std::weak_ptr<vicodyn_dispatch_t> life_cycle)
            -> void;

    /// Upstream
    auto chunk(const hpack::headers_t& headers, std::string&& data) -> bool;
    auto choke(const hpack::headers_t& headers) -> bool;
    auto error(const hpack::headers_t& headers, std::error_code&& ec, std::string&& msg) -> bool;

    /// Dispatch
    template<class Event>
    typename std::enable_if<std::is_same<Event, protocol_t::chunk>::value,
            slot_builder<Event, std::tuple<before_t>>>::type
    on() {
        before_t set_started(std::bind(&endpoint_t::set_started, this));
        return backward_dispatch_.on<Event>().with_middleware(std::move(set_started));
    }

    template<class Event>
    typename std::enable_if<std::is_same<Event, protocol_t::choke>::value,
            slot_builder<Event, std::tuple<before_t, before_t>>>::type
    on() {
        before_t set_started(std::bind(&endpoint_t::set_started, this));
        before_t store_elapsed_time(std::bind(&endpoint_t::store_elapsed_time, this));
        return backward_dispatch_.on<Event>()
                .with_middleware(std::move(store_elapsed_time))
                .with_middleware(std::move(set_started));
    }

    template<class Event>
    typename std::enable_if<std::is_same<Event, protocol_t::error>::value,
            slot_builder<Event, std::tuple<pre_check_error_t>>>::type
    on() {
        namespace ph = std::placeholders;
        pre_check_error_t check_error(std::bind(&endpoint_t::send_error_backward, this, ph::_1, ph::_2));
        return backward_dispatch_.on<Event>().with_middleware(std::move(check_error));
    }

    auto on_discard(discardable_dispatch_t::discarder_t discarder) -> void;

private:
    /// Middlewares
    auto set_started() -> void;
    auto send_error_backward(const std::error_code& ec, const std::string& msg) -> bool;
    auto store_elapsed_time() -> void;

    auto choose_endpoint(const hpack::headers_t& headers, std::string&& event) -> bool;
    auto retry(bool with_reconnect) -> void;

    template<typename F>
    auto catch_connection_error(F f) -> bool;
};

} // namespace vicodyn
} // namespace cocaine
