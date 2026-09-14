/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "schema-properties.h"

#include <array>
#include <string>

namespace webifc::schema
{
  // schema-names.h is generated without an include guard, a namespace or inline
  // definitions, so it can only be compiled into a single translation unit.
  // Keeping it internal lets the compiler drop the lookups nothing calls yet;
  // expose one forwarder per function that is actually needed.
  namespace generated
  {
    namespace
    {
#include "schema-names.h"
    }
  }

  uint32_t getPropertyCount(IFC_SCHEMA schema, uint32_t typeCode)
  {
    return generated::getPropertyCount(schema, typeCode);
  }
}
