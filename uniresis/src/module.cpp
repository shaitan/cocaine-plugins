#include <cocaine/repository.hpp>
#include <cocaine/repository/service.hpp>

#include "cocaine/service/uniresis.hpp"
#include "cocaine/uniresis/error.hpp"

extern "C" {

auto
validation() -> cocaine::api::preconditions_t {
    return cocaine::api::preconditions_t{COCAINE_MAKE_VERSION(0, 12, 14)};
}

auto
initialize(cocaine::api::repository_t& repository) -> void {
    auto id = std::hash<std::string>{}("uniresis");
    cocaine::error::registrar::add(cocaine::uniresis::uniresis_category(), id);

    repository.insert<cocaine::service::uniresis_t>("uniresis");
}

}
