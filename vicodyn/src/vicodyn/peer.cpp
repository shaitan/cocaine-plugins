#include "cocaine/vicodyn/peer.hpp"

#include "cocaine/format/endpoint.hpp"
#include "cocaine/format/peer.hpp"

#include <cocaine/context.hpp>
#include <cocaine/engine.hpp>
#include <cocaine/errors.hpp>
#include <cocaine/logging.hpp>
#include <cocaine/rpc/asio/decoder.hpp>
#include <cocaine/rpc/graph.hpp>
#include <cocaine/rpc/session.hpp>
#include <cocaine/memory.hpp>

#include <asio/ip/tcp.hpp>
#include <asio/connect.hpp>

#include <blackhole/logger.hpp>

#include <metrics/registry.hpp>
#include <cocaine/rpc/upstream.hpp>

namespace cocaine {
namespace vicodyn {

peer_t::~peer_t(){
    session_.apply([&](std::shared_ptr<session_t>& session) {
        if (session) {
            session->detach(std::error_code());
        }
    });
}

peer_t::peer_t(context_t& context, asio::io_service& loop, endpoints_t endpoints, std::string uuid, dynamic_t::object_t extra) :
    context_(context),
    loop_(loop),
    timer_(loop),
    logger_(context.log(format("vicodyn_peer/{}", uuid))),
    d_{
        std::move(uuid),
        std::move(endpoints),
        std::chrono::system_clock::now(),
        std::move(extra),
        /*x_cocaine_cluster*/ {},
        /*hostname*/ {},
    }
{
    d_.x_cocaine_cluster = d_.extra.at("x-cocaine-cluster", "").as_string();
    d_.hostname = [&]() -> std::string {
        asio::io_service asio;
        asio::ip::tcp::resolver resolver(asio);
        const asio::ip::tcp::resolver::iterator end;

        for (const auto& endpoint: d_.endpoints) {
            auto it = resolver.resolve(endpoint);
            if (it != end) {
                return it->host_name();
            }
        }
        return {};
    }();
}

auto peer_t::schedule_reconnect() -> void {
    COCAINE_LOG_INFO(logger_, "scheduling reconnection of peer {} to {}", uuid(), endpoints());
    session_.apply([&](std::shared_ptr<session_t>& session) {
        schedule_reconnect(session);
    });
}
auto peer_t::schedule_reconnect(std::shared_ptr<cocaine::session_t>& session) -> void {
    if (connecting_) {
        COCAINE_LOG_INFO(logger_, "reconnection is already in progress for {}", uuid());
        return;
    }
    if (session) {
        // In fact it should be detached already
        session->detach(std::error_code());
        session = nullptr;
    }
    timer_.expires_from_now(boost::posix_time::seconds(1));
    timer_.async_wait([&](std::error_code ec) {
        if (!ec) {
            connect();
        }
    });
    connecting_ = true;
    COCAINE_LOG_INFO(logger_, "scheduled reconnection of peer {} to {}", uuid(), endpoints());
}

auto peer_t::connect() -> void {
    connecting_ = true;
    COCAINE_LOG_INFO(logger_, "connecting peer {} to {}", uuid(), endpoints());

    auto socket = std::make_shared<asio::ip::tcp::socket>(loop_);
    auto connect_timer = std::make_shared<asio::deadline_timer>(loop_);

    std::weak_ptr<peer_t> weak_self(shared_from_this());

    auto begin = d_.endpoints.begin();
    auto end = d_.endpoints.end();

    connect_timer->expires_from_now(boost::posix_time::seconds(60));
    connect_timer->async_wait([=](std::error_code ec) {
        auto self = weak_self.lock();
        if (!self){
            return;
        }
        if (!ec) {
            COCAINE_LOG_INFO(logger_, "connection timer expired, canceling socket, going to schedule reconnect");
            socket->cancel();
            self->connecting_ = false;
            self->schedule_reconnect();
        } else {
            COCAINE_LOG_DEBUG(logger_, "connection timer was cancelled");
        }
    });

    asio::async_connect(*socket, begin, end, [=](const std::error_code& ec, endpoints_t::const_iterator endpoint_it) {
        auto self = weak_self.lock();
        if (!self){
            return;
        }
        COCAINE_LOG_DEBUG(logger_, "cancelling timer");
        if (!connect_timer->cancel()) {
            COCAINE_LOG_ERROR(logger_, "could not connect to {} - timed out (timer could not be cancelled)", *endpoint_it);
            return;
        }
        if (ec) {
            COCAINE_LOG_ERROR(logger_, "could not connect to {} - {}({})", *endpoint_it, ec.message(), ec.value());
            if (endpoint_it == end) {
                connecting_ = false;
                schedule_reconnect();
            }
            return;
        }
        try {
            COCAINE_LOG_INFO(logger_, "successfully connected peer {} to {}", uuid(), endpoints());
            auto ptr = std::make_unique<asio::ip::tcp::socket>(std::move(*socket));
            auto new_session = context_.engine().attach(std::move(ptr), nullptr);
            session_.apply([&](std::shared_ptr<session_t>& session) {
                connecting_ = false;
                session = std::move(new_session);
                d_.last_active = std::chrono::system_clock::now();
            });
        } catch(const std::exception& e) {
            COCAINE_LOG_WARNING(logger_, "failed to attach session to queue: {}", e.what());
            schedule_reconnect();
        }
    });
}

auto peer_t::uuid() const -> const std::string& {
    return d_.uuid;
}

auto peer_t::hostname() const -> const std::string& {
    return d_.hostname;
}

auto peer_t::endpoints() const -> const std::vector<asio::ip::tcp::endpoint>& {
    return d_.endpoints;
}

auto peer_t::connected() const -> bool {
    return session_.apply([&](const std::shared_ptr<session_t>& session){
        return static_cast<bool>(session);
    });
}

auto peer_t::last_active() const -> std::chrono::system_clock::time_point {
    return d_.last_active;
}

auto peer_t::extra() const -> const dynamic_t::object_t& {
    return d_.extra;
}

auto peer_t::x_cocaine_cluster() const -> const std::string& {
    return d_.x_cocaine_cluster;
}

peers_t::peers_t(context_t& context):
    context(context),
    logger(context.log("vicodyn/peers_t"))
{}

auto peers_t::register_peer(const std::string& uuid, const endpoints_t& endpoints, dynamic_t::object_t extra)
    -> std::shared_ptr<peer_t>
{
    return apply([&](data_t& data){
        auto& peer = data.peers[uuid];
        if (!peer) {
            peer = std::make_shared<peer_t>(context, executor.asio(), endpoints, uuid, std::move(extra));
            peer->connect();
        } else if (endpoints != peer->endpoints()) {
            COCAINE_LOG_ERROR(logger, "changed endpoints detected for uuid {}, previous {}, new {}", uuid,
                              peer->endpoints(), endpoints);
            peer = std::make_shared<peer_t>(context, executor.asio(), endpoints, uuid, extra);
            peer->connect();
        }
        return peer;
    });
}

auto peers_t::register_peer(const std::string& uuid, std::shared_ptr<peer_t> peer) -> void {
    apply([&](data_t& data) {
        data.peers[uuid] = std::move(peer);
    });
}

auto peers_t::erase_peer(const std::string& uuid) -> void {
    apply([&](data_t& data){
        data.peers.erase(uuid);
    });
}

auto peers_t::register_app(const std::string& uuid, const std::string& name) -> void {
    apply([&](data_t& data) {
        data.apps[name].insert(uuid);
    });
}

auto peers_t::erase_app(const std::string& uuid, const std::string& name) -> void {
    apply([&](data_t& data) {
        data.apps[name].erase(uuid);
    });
}

auto peers_t::erase(const std::string& uuid) -> void {
    erase_peer(uuid);
    apply([&](data_t& data) {
        data.peers.erase(uuid);
        for (auto pair : data.apps) {
            pair.second.erase(uuid);
        }
    });
}

auto peers_t::peer(const std::string& uuid) -> std::shared_ptr<peer_t> {
    return apply_shared([&](const data_t& data) -> std::shared_ptr<peer_t>{
        auto it = data.peers.find(uuid);
        if (it != data.peers.end()) {
            return it->second;
        }
        return nullptr;
    });
}

} // namespace vicodyn
} // namespace cocaine
