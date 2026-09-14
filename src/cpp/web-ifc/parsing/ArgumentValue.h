/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace webifc::parsing
{
  struct ArgumentValue
  {
    struct Null {};
    struct Derived {};
    struct String { std::string value; };
    struct Enum { std::string value; };
    struct Ref { uint32_t value; };
    struct Typed { std::string type; std::shared_ptr<ArgumentValue> value; };

    using List = std::vector<ArgumentValue>;
    using Variant = std::variant<Null, Derived, String, Enum, int64_t, double,
                                 Ref, List, Typed>;

    Variant value;

    ArgumentValue(Null v = {}) : value(v) {}
    ArgumentValue(Derived v) : value(v) {}
    ArgumentValue(String v) : value(std::move(v)) {}
    ArgumentValue(Enum v) : value(std::move(v)) {}
    template <std::integral T>
      requires(!std::same_as<T, bool>)
    ArgumentValue(T v) : value(static_cast<int64_t>(v)) {}

    template <std::floating_point T>
    ArgumentValue(T v) : value(static_cast<double>(v)) {}

    ArgumentValue(Ref v) : value(v) {}
    ArgumentValue(List v) : value(std::move(v)) {}
    ArgumentValue(Typed v) : value(std::move(v)) {}
  };
}
