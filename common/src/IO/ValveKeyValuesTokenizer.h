/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "IO/Tokenizer.h"

namespace TrenchBroom {
namespace IO {
namespace ValveKeyValuesToken {
using Type = unsigned int;
static const Type Eof = 1 << 0;
static const Type Key = 1 << 1;
static const Type Value = 1 << 2;
static const Type OBrace = 1 << 3;
static const Type CBrace = 1 << 4;
static const Type ControlStatement = 1 << 5;
} // namespace ValveKeyValuesToken

class ValveKeyValuesTokenizer : public Tokenizer<ValveKeyValuesToken::Type> {
public:
  explicit ValveKeyValuesTokenizer(std::string_view str);

private:
  Token emitToken() override;
  ValveKeyValuesToken::Type consumeKeyOrValue();
  void recordFinalCharacterOnLine(size_t line, size_t column, char input);

  bool m_hasKeyWithoutValue = false;
  bool m_expectNewLine = false;
};
} // namespace IO
} // namespace TrenchBroom
