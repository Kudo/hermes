/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
#include "JSONLexer.h"

#include "hermes/VM/StringPrimitive.h"
#include "hermes/dtoa/dtoa.h"

namespace hermes {
namespace vm {

static const char *TrueString = "true";
static const char *FalseString = "false";
static const char *NullString = "null";

static bool isJSONWhiteSpace(char16_t ch) {
  // JSONWhiteSpace includes <TAB>, <CR>, <LF>, <SP>.
  return (ch == u'\t' || ch == u'\r' || ch == u'\n' || ch == u' ');
}

ExecutionStatus JSONLexer::advance() {
  // Skip whitespaces.
  while (curCharPtr_ < bufferEnd_ && isJSONWhiteSpace(*curCharPtr_)) {
    curCharPtr_++;
  }

  // End of buffer.
  if (curCharPtr_ == bufferEnd_) {
    token_.setEof();
    return ExecutionStatus::RETURNED;
  }

  token_.setLoc(curCharPtr_);

#define PUNC(ch, tok)          \
  case ch:                     \
    token_.setPunctuator(tok); \
    ++curCharPtr_;             \
    return ExecutionStatus::RETURNED

#define WORD(ch, word, tok) \
  case ch:                  \
    return scanWord(word, tok)

  switch (*curCharPtr_) {
    PUNC(u'{', JSONTokenKind::LBrace);
    PUNC(u'}', JSONTokenKind::RBrace);
    PUNC(u'[', JSONTokenKind::LSquare);
    PUNC(u']', JSONTokenKind::RSquare);
    PUNC(u',', JSONTokenKind::Comma);
    PUNC(u':', JSONTokenKind::Colon);
    WORD(u't', TrueString, JSONTokenKind::True);
    WORD(u'f', FalseString, JSONTokenKind::False);
    WORD(u'n', NullString, JSONTokenKind::Null);

    // clang-format off
    case u'-':
    case u'0': case u'1': case u'2': case u'3': case u'4':
    case u'5': case u'6': case u'7': case u'8': case u'9':
      // clang-format on
      return scanNumber();

    case u'"':
      return scanString();

    default:
      return errorWithChar(u"Unexpected token: ", *curCharPtr_);
  }
}

CallResult<char16_t> JSONLexer::consumeUnicode() {
  uint16_t val = 0;
  for (unsigned i = 0; i < 4; ++i) {
    if (curCharPtr_ == bufferEnd_) {
      return error("Unexpected end of input");
    }
    int ch = *curCharPtr_ | 32;
    if (ch >= '0' && ch <= '9') {
      ch -= '0';
    } else if (ch >= 'a' && ch <= 'f') {
      ch -= 'a' - 10;
    } else {
      return errorWithChar(u"Invalid unicode point character: ", *curCharPtr_);
    }
    val = (val << 4) + ch;
    ++curCharPtr_;
  }

  return static_cast<char16_t>(val);
}

ExecutionStatus JSONLexer::scanNumber() {
  const char16_t *start = curCharPtr_;
  while (curCharPtr_ < bufferEnd_) {
    auto ch = *curCharPtr_;
    if (!(ch == u'-' || ch == u'+' || ch == u'.' || (ch | 32) == u'e' ||
          (ch >= u'0' && ch <= u'9'))) {
      break;
    }
    curCharPtr_++;
  }

  size_t len = curCharPtr_ - start;
  if (*start == u'0' && len > 0 && *(start + 1) >= u'0' &&
      *(start + 1) <= u'9') {
    // The integer part cannot start with 0, unless it's 0.
    return errorWithChar(u"Unexpected token in number: ", *(start + 1));
  }

  // copy 16 bit chars into 8 bit chars and call g_strtod.
  llvm::SmallVector<char, 32> str8;
  str8.insert(str8.begin(), start, start + len);
  str8.push_back('\0');

  char *endPtr;
  double value = ::g_strtod(str8.data(), &endPtr);
  if (endPtr != str8.data() + len) {
    return errorWithChar(
        u"Unexpected token in number: ", *(start + (endPtr - str8.data())));
  }
  token_.setNumber(value);
  return ExecutionStatus::RETURNED;
}

ExecutionStatus JSONLexer::scanString() {
  assert(*curCharPtr_ == '"');
  ++curCharPtr_;
  SmallU16String<32> tmpStorage;

  while (curCharPtr_ < bufferEnd_) {
    if (*curCharPtr_ == '"') {
      // End of string.
      auto strRes = StringPrimitive::create(runtime_, tmpStorage.arrayRef());
      if (LLVM_UNLIKELY(strRes == ExecutionStatus::EXCEPTION)) {
        return ExecutionStatus::EXCEPTION;
      }
      token_.setString(runtime_->makeHandle<StringPrimitive>(*strRes));
      ++curCharPtr_;
      return ExecutionStatus::RETURNED;
    } else if (*curCharPtr_ <= '\u001F') {
      return error(u"U+0000 thru U+001F is not allowed in string");
    }
    if (*curCharPtr_ == u'\\') {
      ++curCharPtr_;
      if (curCharPtr_ == bufferEnd_) {
        return error("Unexpected end of input");
      }
      switch (*curCharPtr_) {
        case u'"':
        case u'/':
        case u'\\':
          tmpStorage.push_back(*curCharPtr_++);
          break;

        case 'b':
          ++curCharPtr_;
          tmpStorage.push_back(8);
          break;
        case 'f':
          ++curCharPtr_;
          tmpStorage.push_back(12);
          break;
        case 'n':
          ++curCharPtr_;
          tmpStorage.push_back(10);
          break;
        case 'r':
          ++curCharPtr_;
          tmpStorage.push_back(13);
          break;
        case 't':
          ++curCharPtr_;
          tmpStorage.push_back(9);
          break;

        case 'u': {
          ++curCharPtr_;
          CallResult<char16_t> cr = consumeUnicode();
          if (LLVM_UNLIKELY(cr == ExecutionStatus::EXCEPTION)) {
            return ExecutionStatus::EXCEPTION;
          }
          tmpStorage.push_back(*cr);
          break;
        }

        default:
          return errorWithChar(u"Invalid escape sequence: ", *curCharPtr_);
      }
    } else {
      tmpStorage.push_back(*curCharPtr_++);
    }
  }
  return error("Unexpected end of input");
}

ExecutionStatus JSONLexer::scanWord(const char *word, JSONTokenKind kind) {
  while (*word && curCharPtr_ < bufferEnd_) {
    if (*curCharPtr_ != *word) {
      return errorWithChar(u"Unexpected token: ", *curCharPtr_);
    }
    ++curCharPtr_;
    ++word;
  }
  if (*word) {
    return error(u"Unexpected end of input");
  }
  token_.setPunctuator(kind);
  return ExecutionStatus::RETURNED;
}

}; // namespace vm
}; // namespace hermes
