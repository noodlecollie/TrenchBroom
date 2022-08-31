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

#include "IO/ValveKeyValuesParser.h"
#include "IO/ValveKeyValuesTokenizer.h"
#include "IO/ValveKeyValuesTree.h"
#include "Logger.h"

namespace TrenchBroom {
namespace IO {
ValveKeyValuesParser::ValveKeyValuesParser(std::string_view input)
  : Parser<ValveKeyValuesToken::Type>()
  , m_tokenizer(std::move(input)) {}

void ValveKeyValuesParser::parse(Logger& logger, ValveKeyValuesTree& tree) {
  tree.clear();

  do {
    Token token = m_tokenizer.peekToken();

    if (token.type() == ValveKeyValuesToken::ControlStatement) {
      logger.warn(
        "Valve KeyValues control statement on line " + std::to_string(token.line()) +
        " is not currently supported, ignoring.");

      m_tokenizer.nextToken();
    } else {
      break;
    }
  } while (true);

  parseNodeRecursive(logger, *tree.getRoot());
}

void ValveKeyValuesParser::parseNodeRecursive(Logger& logger, ValveKeyValuesNode& parent) {
  Token token = expect(ValveKeyValuesToken::Key, m_tokenizer.nextToken());

  ValveKeyValuesNode* node = parent.addChild(token.data());

  token = expect(ValveKeyValuesToken::Value | ValveKeyValuesToken::OBrace, m_tokenizer.nextToken());

  if (token.type() == ValveKeyValuesToken::Value) {
    node->setValueString(token.data());
  } else {
    while (m_tokenizer.peekToken().type() != ValveKeyValuesToken::CBrace) {
      parseNodeRecursive(logger, *node);
    }

    // Eat closing brace.
    expect(ValveKeyValuesToken::CBrace, m_tokenizer.nextToken());
  }
}
} // namespace IO
} // namespace TrenchBroom
