/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "ArgumentValue.h"
#include "IfcLoader.h"

namespace webifc::parsing
{
  uint32_t CreateLine(IfcLoader &loader, uint32_t type,
                      const std::vector<ArgumentValue> &args);
  void WriteLine(IfcLoader &loader, uint32_t expressID, uint32_t type,
                 const std::vector<ArgumentValue> &args);
  void SetArguments(IfcLoader &loader, uint32_t expressID,
                    const std::map<uint32_t, ArgumentValue> &args);
  void SetHeaderArguments(IfcLoader &loader, uint32_t type,
                          const std::map<uint32_t, ArgumentValue> &args);
  void DeleteLine(IfcLoader &loader, uint32_t expressID);
}
