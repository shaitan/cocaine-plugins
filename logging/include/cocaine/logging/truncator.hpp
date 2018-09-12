/*
    Copyright (c) 2018 Artem Selishchev <artsel@yandex-team.ru>
    Copyright (c) 2018 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <blackhole/attributes.hpp>
#include <cocaine/dynamic.hpp>

namespace cocaine {
namespace logging {

class truncator_t {
    const std::uint64_t message_size_;
    const std::uint64_t attribute_key_size_;
    const std::uint64_t attribute_value_size_;
    const std::uint64_t attributes_count_;

public:
    truncator_t(const dynamic_t& args);

    auto truncate_message(std::string& msg) const -> void;
    auto truncate_attributes(blackhole::attributes_t& attributes) const -> void;
    auto truncate_attribute_count(blackhole::attribute_list& attribute_list) const -> void;
};

} // namespace logging
} // namespace cocaine
