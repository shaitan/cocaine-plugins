#include "cocaine/vicodyn/balancer/simple.hpp"
#include "cocaine/vicodyn/balancer/utils.hpp"

#include <cocaine/context.hpp>
#include <cocaine/format/peer.hpp>
#include <cocaine/format/ptr.hpp>

namespace cocaine {
namespace vicodyn {
namespace balancer {

simple_t::simple_t(context_t& ctx, peers_t& peers, asio::io_service& loop, const std::string& app_name,
                   const dynamic_t& args, const dynamic_t::object_t& locator_extra) :
    api::vicodyn::balancer_t(ctx, peers, loop, app_name, args, locator_extra),
    peers(peers),
    logger(ctx.log(format("balancer/simple/{}", app_name))),
    args(args),
    _retry_count(args.as_object().at("retry_count", 4u).as_uint()),
    app_name(app_name),
    x_cocaine_cluster(locator_extra.at("x-cocaine-cluster", "").as_string()),
    ban_timeout(args.as_object().at("ban-timeout-ms", 0U).as_uint())
{
    COCAINE_LOG_INFO(logger, "created simple balancer for app {}", app_name);
}

auto simple_t::choose_peer(const std::shared_ptr<request_context_t>& /*request_context*/, const hpack::headers_t& /*headers*/,
                           const std::string& /*event*/) -> std::shared_ptr<cocaine::vicodyn::peer_t>
{
    return peers.apply_shared([&](const peers_t::data_t& mapping) {
        auto apps_it = mapping.apps.find(app_name);
        if(apps_it == mapping.apps.end() || apps_it->second.empty()) {
            COCAINE_LOG_WARNING(logger, "peer list for app {} is empty", app_name);
            throw error_t("no peers found");
        }
        auto& apps = apps_it->second;
        std::vector<peers_t::peers_data_t::const_iterator> chosen;
        chosen.reserve(apps.size());
        for (const auto& app_it : apps) {
            if (app_it.second.banned()) {
                continue;
            }
            auto peer_it = mapping.peers.find(app_it.first);
            if (peer_it == mapping.peers.end()) {
                continue;
            }
            if (!peer_it->second->connected()) {
                continue;
            }
            if (peer_it->second->x_cocaine_cluster() != x_cocaine_cluster) {
                continue;
            }
            chosen.push_back(peer_it);
        }
        auto it = choose_random(chosen.begin(), chosen.end());
        if (it != chosen.end()) {
            return (*it)->second;
        }
        COCAINE_LOG_WARNING(logger, "all peers do not have desired app");
        throw error_t("no peers found");
    });
}

auto simple_t::retry_count() -> size_t {
    return _retry_count;
}

auto simple_t::on_error(const std::shared_ptr<peer_t>& peer, std::error_code ec, const std::string& msg) -> void {
    COCAINE_LOG_WARNING(logger, "peer errored - {}({})", ec.message(), msg);
    if(ec.category() == error::node_category() && ec.value() == error::node_errors::not_running) {
        peers.erase_app(peer->uuid(), app_name);
    }
    if(ec.category() == error::overseer_category() && ec.value() == error::queue_is_full) {
        peers.ban_app(peer->uuid(), app_name, ban_timeout);
        COCAINE_LOG_WARNING(logger, "queue is full, peer banned for {}ms - {}", ban_timeout.count(), peer);
    }
}

auto simple_t::is_recoverable(const std::shared_ptr<peer_t>&, std::error_code ec) -> bool {
    bool queue_is_full = (ec.category() == error::overseer_category() && ec.value() == error::queue_is_full);
    bool unavailable = (ec.category() == error::node_category() && ec.value() == error::not_running);
    bool disconnected = (ec.category() == error::dispatch_category() && ec.value() == error::not_connected);
    return queue_is_full || unavailable || disconnected;
}

} // namespace balancer
} // namespace vicodyn
} // namespace cocaine
