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

#include "IO/ValveKeyValuesTokenizer.h"
#include "Exceptions.h"

namespace TrenchBroom {
namespace IO {
// According to https://developer.valvesoftware.com/wiki/KeyValues#About_KeyValues_Text_File_Format:
// "Non-quoted tokens ends with a whitespace, {, } and ". So you may use { and } within quoted
// tokens, but not for non-quoted tokens."
static const std::string UnquotedStringDelims(" \n\t\r{}\"");

ValveKeyValuesTokenizer::ValveKeyValuesTokenizer(std::string_view str)
  : Tokenizer<ValveKeyValuesToken::Type>(std::move(str), "\n\t\\\"", '\\') {}

Tokenizer<ValveKeyValuesToken::Type>::Token ValveKeyValuesTokenizer::emitToken() {
  while (!eof()) {
    size_t startLine = line();
    size_t startColumn = column();
    const char* firstChar = curPos();

    switch (*firstChar) {
      case '/': {
        advance();

        if (curChar() != '/') {
          throw ParserException(
            startLine, startColumn, "Unexpected character: '" + std::string(firstChar, 1) + "'");
        }

        discardUntil("\r\n");
        m_hasKeyWithoutValue = false;
        m_expectNewLine = false;

        break;
      }

      case '{': {
        recordFinalCharacterOnLine(startLine, startColumn, *firstChar);
        advance();
        m_hasKeyWithoutValue = false;

        return Token(
          ValveKeyValuesToken::OBrace, firstChar, firstChar + 1, offset(firstChar), startLine,
          startColumn);
      }

      case '}': {
        recordFinalCharacterOnLine(startLine, startColumn, *firstChar);
        advance();
        m_hasKeyWithoutValue = false;

        return Token(
          ValveKeyValuesToken::CBrace, firstChar, firstChar + 1, offset(firstChar), startLine,
          startColumn);
      }

      case '"': {
        advance();
        firstChar = curPos();
        const char* quotedString = readQuotedString();
        return Token(
          consumeKeyOrValue(), firstChar, quotedString, offset(firstChar), startLine, startColumn);
      }

      case '#': {
        advance();
        firstChar = curPos();
        const char* str = readUntil("\r\n");
        m_hasKeyWithoutValue = false;
        m_expectNewLine = false;

        return Token(
          ValveKeyValuesToken::ControlStatement, firstChar, str, offset(firstChar), startLine,
          startColumn);
      }

      case ' ':
      case '\t': {
        discardWhile(" \t");
        break;
      }

      case '\n':
      case '\r': {
        discardWhile("\r\n");
        m_hasKeyWithoutValue = false;
        break;
      }

      default: {
        const char* str = readUntil(UnquotedStringDelims);

        if (str) {
          return Token(
            consumeKeyOrValue(), firstChar, str, offset(firstChar), startLine, startColumn);
        } else {
          throw ParserException(
            startLine, startColumn, "Unexpected character: '" + std::string(firstChar, 1) + "'");
        }
        break;
      }
    }
  }

  return Token(ValveKeyValuesToken::Eof, nullptr, nullptr, length(), line(), column());
}

ValveKeyValuesToken::Type ValveKeyValuesTokenizer::consumeKeyOrValue() {
  ValveKeyValuesToken::Type tokenType =
    m_hasKeyWithoutValue ? ValveKeyValuesToken::Value : ValveKeyValuesToken::Key;

  m_hasKeyWithoutValue = !m_hasKeyWithoutValue;

  if (tokenType == ValveKeyValuesToken::Value) {
    m_expectNewLine = true;
  }

  return tokenType;
}

void ValveKeyValuesTokenizer::recordFinalCharacterOnLine(size_t line, size_t column, char input) {
  if (m_expectNewLine) {
    throw ParserException(line, column, "Expected newline but got '" + std::to_string(input) + "'");
  }

  m_expectNewLine = true;
}
} // namespace IO
} // namespace TrenchBroom
