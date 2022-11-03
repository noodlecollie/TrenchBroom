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
        break;
      }

      case '{': {
        advance();

        return Token(
          ValveKeyValuesToken::OBrace, firstChar, firstChar + 1, offset(firstChar), startLine,
          startColumn);
      }

      case '}': {
        advance();

        return Token(
          ValveKeyValuesToken::CBrace, firstChar, firstChar + 1, offset(firstChar), startLine,
          startColumn);
      }

      case '"': {
        advance();
        firstChar = curPos();
        const char* quotedString = readQuotedString();
        return Token(
          ValveKeyValuesToken::String, firstChar, quotedString, offset(firstChar), startLine,
          startColumn);
      }

      case '#': {
        advance();
        firstChar = curPos();
        const char* str = readUntil("\r\n");
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
        const char* str = readWhile("\r\n");
        return Token(
          ValveKeyValuesToken::NewLine, firstChar, str, offset(firstChar), startLine, startColumn);
        break;
      }

      default: {
        const char* str = readUntil(UnquotedStringDelims);

        if (str) {
          return Token(
            ValveKeyValuesToken::String, firstChar, str, offset(firstChar), startLine, startColumn);
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
} // namespace IO
} // namespace TrenchBroom
