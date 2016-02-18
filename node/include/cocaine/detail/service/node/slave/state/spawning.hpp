#pragma once

#include <asio/deadline_timer.hpp>

#include "state.hpp"
#include "handshaking.hpp"

namespace cocaine {

namespace api {
    struct handle_t;
} // namespace api

class state_machine_t;

class spawning_t:
    public state_t,
    public std::enable_shared_from_this<spawning_t>
{
    std::shared_ptr<state_machine_t> slave;

    asio::deadline_timer timer;
    std::unique_ptr<api::handle_t> handle;

    struct data_t {
        std::shared_ptr<session_t> session;
        std::shared_ptr<control_t> control;
    };

    synchronized<data_t> data;

public:
    explicit
    spawning_t(std::shared_ptr<state_machine_t> slave);

    virtual
    const char*
    name() const noexcept;

    virtual
    void
    cancel();

    virtual
    std::shared_ptr<control_t>
    activate(std::shared_ptr<session_t> session, upstream<io::worker::control_tag> stream);

    virtual
    void
    terminate(const std::error_code& ec);

    void
    spawn(unsigned long timeout);

private:
    void
    on_spawn(std::chrono::high_resolution_clock::time_point start);

    void
    on_timeout(const std::error_code& ec);
};

}
