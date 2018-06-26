#include "cocaine/vicodyn/request_context.hpp"

#include <cocaine/logging.hpp>

namespace cocaine {
namespace vicodyn {

request_context_t::request_context_t(blackhole::logger_t& logger) :
    logger_(logger),
    start_time_(clock_t::now()),
    closed_(ATOMIC_FLAG_INIT),
    retry_counter_(0)
{}

request_context_t::~request_context_t() {
    fail({}, "dtor called");
}

auto request_context_t::mark_used_peer(std::shared_ptr<peer_t> peer) -> void {
    used_peers_->emplace_back(std::move(peer));
}

auto request_context_t::peer_use_count(const std::shared_ptr<peer_t>& peer) -> size_t {
    return used_peers_.apply([&](const std::vector<std::shared_ptr<peer_t>>& used_peers_){
        return std::count(used_peers_.begin(), used_peers_.end(), peer);
    });
}

auto request_context_t::peer_use_count(const std::string& peer_uuid) -> size_t {
    return used_peers_.apply([&](const std::vector<std::shared_ptr<peer_t>>& used_peers){
        return std::count_if(used_peers.begin(), used_peers.end(), [&](const std::shared_ptr<peer_t>& peer){
            return peer->uuid() == peer_uuid;
        });
    });
}

auto request_context_t::register_retry() -> void {
    retry_counter_++;
}

auto request_context_t::retry_count() -> size_t {
    return retry_counter_;
}

auto request_context_t::custom_context() -> synchronized<boost::any>& {
    return custom_context_;
}

auto request_context_t::finish() -> void {
    static std::string msg("finished request");
    write(logging::info, "finished request");
}

auto request_context_t::fail(const std::error_code& ec, blackhole::string_view reason) -> void {
    write(logging::warning, format("finished request with error {} - {}", ec, reason));
}

auto request_context_t::current_duration_ms() -> size_t {
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(clock_t::now() - start_time_);
    return dur.count();
}

auto request_context_t::write(int level, const std::string& msg) -> void {
    if(closed_.test_and_set()) {
        return;
    }
    add_checkpoint("total_duration_ms");

    blackhole::view_of<blackhole::attributes_t>::type view;
    auto sync_peers = used_peers_.synchronize();
    auto sync_checkpoints = checkpoints_.synchronize();
    for (const auto& peer: *sync_peers) {
        view.emplace_back("peer", peer->uuid());
    }
    view.emplace_back("retry_cnt", retry_counter_);
    for (const auto& checkpoint: *sync_checkpoints) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(checkpoint.when - start_time_);
        view.emplace_back(blackhole::string_view(checkpoint.message, checkpoint.msg_len), ms.count());
    }

    COCAINE_LOG(logger_, logging::priorities(level), msg, view);
}

} // namespace vicodyn
} // namespace cocaine
