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
#include "Exceptions.h"
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

  try {
    parseRootNode(logger, *tree.getRoot());
  } catch (const ParserException& ex) {
    tree.clear();
    throw ex;
  }
}

void ValveKeyValuesParser::parseRootNode(Logger& logger, ValveKeyValuesNode& parent) {
  // While we still have a token that can be treated as a key, parse recursively.
  while (m_tokenizer.peekToken(ValveKeyValuesToken::NewLine).type() ==
         ValveKeyValuesToken::String) {
    parseNodeRecursive(logger, parent);
  }

  // Make sure we got to the end of the file.
  expect(ValveKeyValuesToken::Eof, m_tokenizer.nextToken(ValveKeyValuesToken::NewLine));
}

void ValveKeyValuesParser::parseNodeRecursive(Logger& logger, ValveKeyValuesNode& parent) {
  // Initial key string, preceded by any number of newlines.
  Token token =
    expect(ValveKeyValuesToken::String, m_tokenizer.nextToken(ValveKeyValuesToken::NewLine));

  // Create a node based on this key.
  ValveKeyValuesNode* node = parent.addChild(token.data());

  // Expect either a string, or an newline (after which should be an open brace).
  token =
    expect(ValveKeyValuesToken::String | ValveKeyValuesToken::NewLine, m_tokenizer.nextToken());

  if (token.type() == ValveKeyValuesToken::String) {
    // Node has a single value.
    node->setValueString(token.data());
  } else {
    // Make sure we have an opening brace, discarding any further newlines.
    expect(ValveKeyValuesToken::OBrace, m_tokenizer.nextToken(ValveKeyValuesToken::NewLine));

    // Recursively parse children until a closing brace is bound.
    while (m_tokenizer.peekToken(ValveKeyValuesToken::NewLine).type() !=
           ValveKeyValuesToken::CBrace) {
      parseNodeRecursive(logger, *node);
    }

    // Eat closing brace, preceded by any number of newlines.
    expect(ValveKeyValuesToken::CBrace, m_tokenizer.nextToken(ValveKeyValuesToken::NewLine));
  }
}

ValveKeyValuesParser::TokenNameMap ValveKeyValuesParser::tokenNames() const {
  TokenNameMap result;

  result[ValveKeyValuesToken::Eof] = "end of file";
  result[ValveKeyValuesToken::String] = "string";
  result[ValveKeyValuesToken::OBrace] = "{";
  result[ValveKeyValuesToken::CBrace] = "}";
  result[ValveKeyValuesToken::NewLine] = "newline";
  result[ValveKeyValuesToken::ControlStatement] = "#...";

  return result;
}
} // namespace IO
} // namespace TrenchBroom
