/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "IfcLineWriter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "IfcTokenStream.h"
#include "step_number.h"
#include "string_parsing.h"
#include "../schema/IfcSchemaManager.h"
#include "../schema/schema-properties.h"

namespace webifc::parsing
{
  namespace
  {
    constexpr size_t maxArgumentDepth = 8;
    constexpr std::string_view unknownTypeName = "<web-ifc-type-unknown>";

    template <typename T>
    void append(std::vector<uint8_t> &out, T value)
    {
      static_assert(std::is_trivially_copyable_v<T>);
      const auto *bytes = reinterpret_cast<const uint8_t *>(&value);
      out.insert(out.end(), bytes, bytes + sizeof(T));
    }

    void appendBytes(std::vector<uint8_t> &out, std::string_view value)
    {
      out.insert(out.end(), value.begin(), value.end());
    }

    void appendToken(std::vector<uint8_t> &out, IfcTokenType token)
    {
      append<uint8_t>(out, static_cast<uint8_t>(token));
    }

    void appendStringToken(std::vector<uint8_t> &out, IfcTokenType token,
                           std::string_view value)
    {
      if (value.size() > std::numeric_limits<uint16_t>::max())
      {
        throw std::length_error("IFC token payload exceeds 65535 bytes");
      }
      appendToken(out, token);
      append<uint16_t>(out, static_cast<uint16_t>(value.size()));
      appendBytes(out, value);
    }

    std::string toUpperAscii(std::string_view input)
    {
      std::string output;
      output.reserve(input.size());
      for (const char character : input)
      {
        if (character >= 'a' && character <= 'z')
        {
          output.push_back(static_cast<char>(character - 'a' + 'A'));
        }
        else
        {
          output.push_back(character);
        }
      }
      return output;
    }

    bool isIdentifier(std::string_view value)
    {
      if (value.empty() || !((value.front() >= 'A' && value.front() <= 'Z') ||
                             (value.front() >= 'a' && value.front() <= 'z')))
      {
        return false;
      }
      return std::all_of(value.begin() + 1, value.end(), [](char character) {
        return (character >= 'A' && character <= 'Z') ||
               (character >= 'a' && character <= 'z') ||
               (character >= '0' && character <= '9') || character == '_';
      });
    }

    std::string encodeString(std::string_view value)
    {
      std::ostringstream output;
      p21encode(value, output);
      return output.str();
    }

    void validateArgument(const ArgumentValue &value, const IfcLoader &loader,
                          size_t depth = 0)
    {
      if (depth > maxArgumentDepth)
      {
        throw std::invalid_argument("Argument value nesting is too deep");
      }

      std::visit(
          [&loader, depth](const auto &item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, ArgumentValue::Enum>)
            {
              if (!isIdentifier(item.value))
              {
                throw std::invalid_argument("Invalid enumeration value");
              }
            }
            else if constexpr (std::is_same_v<T, double>)
            {
              if (!std::isfinite(item))
              {
                throw std::invalid_argument("Real argument value must be finite");
              }
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::Ref>)
            {
              if (!loader.IsValidExpressID(item.value))
              {
                throw std::invalid_argument("Referenced instance ID not found");
              }
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::List>)
            {
              for (const auto &child : item)
              {
                validateArgument(child, loader, depth + 1);
              }
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::Typed>)
            {
              if (!item.value || !isIdentifier(item.type))
              {
                throw std::invalid_argument("Invalid typed argument value");
              }
              validateArgument(*item.value, loader, depth + 1);
            }
          },
          value.value);
    }

    void appendArgument(std::vector<uint8_t> &out, const ArgumentValue &value)
    {
      std::visit(
          [&out](const auto &item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, ArgumentValue::Null>)
            {
              appendToken(out, IfcTokenType::EMPTY);
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::Derived>)
            {
              appendToken(out, IfcTokenType::UNKNOWN);
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::String>)
            {
              const std::string encoded = encodeString(item.value);
              appendStringToken(out, IfcTokenType::STRING, encoded);
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::Enum>)
            {
              appendStringToken(out, IfcTokenType::ENUM, toUpperAscii(item.value));
            }
            else if constexpr (std::is_same_v<T, int64_t>)
            {
              appendStringToken(out, IfcTokenType::INTEGER,
                                FormatStepInteger(item));
            }
            else if constexpr (std::is_same_v<T, double>)
            {
              appendStringToken(out, IfcTokenType::REAL, FormatStepReal(item));
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::Ref>)
            {
              appendToken(out, IfcTokenType::REF);
              append<uint32_t>(out, item.value);
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::List>)
            {
              appendToken(out, IfcTokenType::SET_BEGIN);
              for (const auto &child : item)
              {
                appendArgument(out, child);
              }
              appendToken(out, IfcTokenType::SET_END);
            }
            else if constexpr (std::is_same_v<T, ArgumentValue::Typed>)
            {
              const std::string type = toUpperAscii(item.type);
              appendStringToken(out, IfcTokenType::LABEL, type);
              appendToken(out, IfcTokenType::SET_BEGIN);
              appendArgument(out, *item.value);
              appendToken(out, IfcTokenType::SET_END);
            }
          },
          value.value);
    }

    void copyExistingArgument(IfcLoader &loader, std::vector<uint8_t> &out,
                              size_t depth = 0)
    {
      if (depth > maxArgumentDepth)
      {
        throw std::invalid_argument("IFC attribute nesting is too deep");
      }
      if (loader.IsAtEnd())
      {
        throw std::invalid_argument("Unexpected end of IFC attribute");
      }

      const IfcTokenType token = loader.GetTokenType();
      appendToken(out, token);
      switch (token)
      {
      case IfcTokenType::REF:
        loader.StepBack();
        append<uint32_t>(out, loader.GetRefArgument());
        return;
      case IfcTokenType::STRING:
      case IfcTokenType::LABEL:
      case IfcTokenType::ENUM:
      case IfcTokenType::REAL:
      case IfcTokenType::INTEGER:
      {
        loader.StepBack();
        const std::string_view payload = loader.GetStringArgument();
        append<uint16_t>(out, static_cast<uint16_t>(payload.size()));
        appendBytes(out, payload);
        if (token == IfcTokenType::LABEL)
        {
          const IfcTokenType payloadToken = loader.GetTokenType();
          loader.StepBack();
          if (payloadToken != IfcTokenType::SET_BEGIN)
          {
            throw std::invalid_argument("Malformed typed IFC attribute");
          }
          copyExistingArgument(loader, out, depth + 1);
        }
        return;
      }
      case IfcTokenType::SET_BEGIN:
        while (true)
        {
          if (loader.IsAtEnd())
          {
            throw std::invalid_argument("Unexpected end of IFC attribute list");
          }
          const IfcTokenType child = loader.GetTokenType();
          loader.StepBack();
          if (child == IfcTokenType::LINE_END)
          {
            throw std::invalid_argument("Malformed IFC attribute list");
          }
          copyExistingArgument(loader, out, depth + 1);
          if (child == IfcTokenType::SET_END)
          {
            return;
          }
        }
      case IfcTokenType::UNKNOWN:
      case IfcTokenType::EMPTY:
      case IfcTokenType::SET_END:
      case IfcTokenType::LINE_END:
        return;
      default:
        throw std::invalid_argument("Unexpected token while copying IFC attribute");
      }
    }

    std::string typeName(uint32_t type)
    {
      static const schema::IfcSchemaManager schemaManager;
      const std::string name = schemaManager.IfcTypeCodeToType(type);
      if (name == unknownTypeName)
      {
        throw std::invalid_argument("Invalid IFC type code");
      }
      return toUpperAscii(name);
    }

    // Starts a line: REF <expressID> LABEL <name> SET_BEGIN.
    std::vector<uint8_t> beginLine(uint32_t expressID, const std::string &name)
    {
      std::vector<uint8_t> line;
      line.reserve(128);
      appendToken(line, IfcTokenType::REF);
      append<uint32_t>(line, expressID);
      appendStringToken(line, IfcTokenType::LABEL, name);
      appendToken(line, IfcTokenType::SET_BEGIN);
      return line;
    }

    // Closes a line and writes it to the tape.
    void pushLine(IfcLoader &loader, uint32_t expressID, uint32_t type,
                  std::vector<uint8_t> &line)
    {
      appendToken(line, IfcTokenType::SET_END);
      appendToken(line, IfcTokenType::LINE_END);

      const uint64_t start = loader.GetTotalSize();
      if (start > std::numeric_limits<uint32_t>::max() ||
          static_cast<uint64_t>(line.size()) >
              std::numeric_limits<uint32_t>::max() - start)
      {
        throw std::overflow_error("IFC tape exceeds uint32_t offsets");
      }

      loader.Push(line.data(), line.size());
      loader.UpdateLineTape(expressID, type, static_cast<uint32_t>(start));
    }

    std::vector<uint8_t> buildLine(IfcLoader &loader, uint32_t expressID,
                                   uint32_t type,
                                   const std::vector<ArgumentValue> &args)
    {
      if (expressID == 0)
      {
        throw std::invalid_argument("Express ID must be greater than zero");
      }
      const std::string name = typeName(type);

      const auto schemaHeaders = loader.GetHeaderLinesWithType(schema::FILE_SCHEMA);
      if (schemaHeaders.empty())
      {
        throw std::invalid_argument("IFC file is missing a FILE_SCHEMA header");
      }
      if (type == schema::FILE_SCHEMA || type == schema::FILE_NAME ||
          type == schema::FILE_DESCRIPTION)
      {
        throw std::invalid_argument("IFC header lines cannot be written as data");
      }
      if (args.size() != schema::getPropertyCount(loader.GetSchema(), type))
      {
        throw std::invalid_argument("Argument count does not match IFC schema");
      }
      for (const auto &argument : args)
      {
        validateArgument(argument, loader);
      }

      std::vector<uint8_t> line = beginLine(expressID, name);
      for (const auto &argument : args)
      {
        appendArgument(line, argument);
      }
      return line;
    }
  }

  uint32_t CreateLine(IfcLoader &loader, uint32_t type,
                      const std::vector<ArgumentValue> &args)
  {
    const uint32_t maxExpressID = loader.GetMaxExpressId();
    if (maxExpressID == std::numeric_limits<uint32_t>::max())
    {
      throw std::overflow_error("No express IDs remain");
    }
    const uint32_t expressID = maxExpressID + 1;
    WriteLine(loader, expressID, type, args);
    return expressID;
  }

  void WriteLine(IfcLoader &loader, uint32_t expressID, uint32_t type,
                 const std::vector<ArgumentValue> &args)
  {
    if (loader.IsValidExpressID(expressID) &&
        loader.GetLineType(expressID) != type)
    {
      throw std::invalid_argument("Cannot change an existing IFC line type");
    }

    std::vector<uint8_t> line = buildLine(loader, expressID, type, args);
    pushLine(loader, expressID, type, line);
  }

  void SetArguments(IfcLoader &loader, uint32_t expressID,
                    const std::map<uint32_t, ArgumentValue> &args)
  {
    if (!loader.IsValidExpressID(expressID))
    {
      throw std::invalid_argument("IFC line not found");
    }
    if (args.empty())
    {
      return;
    }

    const uint32_t type = loader.GetLineType(expressID);
    const uint32_t count = loader.GetNoLineArguments(expressID);
    if (count != schema::getPropertyCount(loader.GetSchema(), type))
    {
      throw std::invalid_argument("Argument count does not match IFC schema");
    }
    for (const auto &[index, argument] : args)
    {
      if (index >= count)
      {
        throw std::invalid_argument("Argument index out of range");
      }
      validateArgument(argument, loader);
    }

    std::vector<uint8_t> line = beginLine(expressID, typeName(type));

    loader.MoveToArgumentOffset(expressID, 0);
    for (uint32_t index = 0; index < count; ++index)
    {
      const size_t offset = line.size();
      copyExistingArgument(loader, line);
      const auto replacement = args.find(index);
      if (replacement != args.end())
      {
        line.resize(offset);
        appendArgument(line, replacement->second);
      }
    }

    pushLine(loader, expressID, type, line);
  }

  void DeleteLine(IfcLoader &loader, uint32_t expressID)
  {
    loader.RemoveLine(expressID);
  }
}
