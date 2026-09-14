/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "step_number.h"

#include <cmath>
#include <format>

namespace webifc::parsing
{
   std::string FormatStepReal(double input)
   {
      std::string numberString = std::format("{}", input);
      size_t eLoc = numberString.find_first_of('e');
      if (eLoc != std::string::npos) numberString[eLoc] = 'E';
      else if (std::floor(input) == input) numberString += '.';
      return numberString;
   }

   std::string FormatStepInteger(int64_t input)
   {
      return std::to_string(input);
   }
}
