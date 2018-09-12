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

#include "cocaine/logging/truncator.hpp"

namespace cocaine {
namespace logging {
namespace defaults {

constexpr auto string_size_limit = std::numeric_limits<std::size_t>::max();

} // namespace defaults

namespace {

auto truncate_inplace(std::string& str, std::size_t size) -> void {
    if (str.size() <= size) {
        return;
    }

    static const std::string ellipsis = "...";
    if (size <= ellipsis.size()) {
        str.replace(0, size, ellipsis);
    }
    else if (size <= 2 * ellipsis.size()) {
        str.replace(size - ellipsis.size(), ellipsis.size(), ellipsis);
    } else {
        const auto left_size = size / 2;
        const auto right_size = (size + 1) / 2 - ellipsis.size();

        str.replace(left_size, ellipsis.size(), ellipsis);
        str.replace(left_size + ellipsis.size(), right_size, str, str.size() - right_size, right_size);
    }
    str.resize(size);
}

auto truncate(const std::string& str, std::size_t size) -> std::string {
    if (str.size() <= size) {
        return str;
    }

    std::string result;
    result.reserve(size);

    static const std::string ellipsis = "...";
    if (size <= ellipsis.size()) {
        result = ellipsis.substr(0, size);
    }
    else if (size <= 2 * ellipsis.size()) {
        result.append(str, 0, size - ellipsis.size());
        result.append(ellipsis);
    } else {
        const auto left_size = size / 2;
        const auto right_size = (size + 1) / 2 - ellipsis.size();

        result.append(str, 0, left_size);
        result.append(ellipsis);
        result.append(str, size - right_size, right_size);
    }
    return result;
}

} // namespace

truncator_t::truncator_t(const dynamic_t& args)
    : message_size_(args.as_object().at("message_size", defaults::string_size_limit).as_uint())
    , attribute_key_size_(args.as_object().at("attribute_key_size", defaults::string_size_limit).as_uint())
    , attribute_value_size_(args.as_object().at("attribute_value_size", defaults::string_size_limit).as_uint())
    , attributes_count_(args.as_object().at("attributes_count", defaults::string_size_limit).as_uint()) {
}

auto truncator_t::truncate_message(std::string& msg) const -> void {
    truncate_inplace(msg, message_size_);
}

auto truncator_t::truncate_attributes(blackhole::attributes_t& attributes) const -> void {
    for (auto& attr : attributes) {
        truncate_inplace(attr.first, attribute_key_size_);
        try {
            const auto& value = blackhole::attribute::get<blackhole::attribute::value_t::string_type>(attr.second);
            attr.second = truncate(value, attribute_value_size_);
        } catch(const std::bad_cast&) {
        }
    }
}

auto truncator_t::truncate_attribute_count(blackhole::attribute_list& attribute_list) const -> void {
    if (attribute_list.size() > attributes_count_) {
        const std::size_t prev_size = attribute_list.size();
        attribute_list.resize(attributes_count_);
        attribute_list.push_back({"truncated", prev_size - attributes_count_});
    }
}

} // namespace logging
} // namespace cocaine
