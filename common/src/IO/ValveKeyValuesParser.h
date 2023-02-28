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

#include "IO/Parser.h"
#include "IO/ValveKeyValuesTokenizer.h"
#include "IO/ValveKeyValuesTree.h"
#include <string>

namespace TrenchBroom {
class Logger;

namespace IO {
class ValveKeyValuesTree;

class ValveKeyValuesParser : public Parser<ValveKeyValuesToken::Type> {
public:
  explicit ValveKeyValuesParser(std::string_view input);

  void parse(Logger& logger, ValveKeyValuesTree& tree);

private:
  using Token = ValveKeyValuesTokenizer::Token;

  void parseRootNode(Logger& logger, ValveKeyValuesNode& parent);
  void parseNodeRecursive(Logger& logger, ValveKeyValuesNode& parent);
  TokenNameMap tokenNames() const override;

  ValveKeyValuesTokenizer m_tokenizer;
};
} // namespace IO
} // namespace TrenchBroom