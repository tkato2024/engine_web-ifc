/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <sstream>
#include <string_view>

namespace webifc::parsing
{
  void p21encode(std::string_view input, std::ostringstream &output);
}
