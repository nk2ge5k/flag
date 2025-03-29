// Copyright 2025, Geogii Chernukhin <nk2ge5k@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef INI_PARSE_H
#define INI_PARSE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INI_MAX_KEY_SIZE
#define INI_MAX_KEY_SIZE 128
#endif

#ifndef INI_MAX_LINE_SIZE
#define INI_MAX_LINE_SIZE 512
#endif

typedef enum {
  INI_ERROR_CODE_NONE = 0,
  // Buffer overflow - key or value size is greater then maximum allowed size
  INI_ERROR_CODE_OVERFLOW = -1,
  // Invalid syntax
  INI_ERROR_INVALID_SYNTAX = -2,
} IniErrorCode;

typedef struct IniParser IniParser;

// iniParserNew creates new parser for the given file handler.
IniParser* iniParserNew(FILE* file);
// iniParserOpen opens given file for reading and creates new parser.
// Returns NULL in case of error.
IniParser* iniParserOpen(const char* filename);
// iniParserFree closes the INI parser and frees allocated resources.
// @note: If IniParser was created using iniParserNew, the file will not be closed.
void iniParserFree(IniParser* parser);

// iniParseKey copies the next key into dst, including the terminating null byte ('\0').
// If the key is longer than maxlen - 1, it is truncated to fit this length.
// Returns the length of the value or zero if no key is found.
int iniParseKey(IniParser* parser, char* dst, int maxlen);
// iniParseValue copies the value for the key into dst, including the terminating null byte ('\0').
// Returns the length of the value or zero if no key is found.
// @note: iniParseValue does not handle incorrect call order - it must be called
// only after iniParseKey.
int iniParseValue(IniParser* parser, char* dst, int maxlen);

#ifdef __cplusplus
}
#endif

#ifdef INI_IMPLEMENTATION

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
# define CAST(type, v) static_cast<type>(v)
#else
# define CAST(type, v) (type)(v)
#endif

static size_t stringLength(const char *s, size_t maxlen) {
  for (size_t i = 0; i < maxlen; i++) {
    if (s[i] == '\0') return i;
  }
  return maxlen;
}

static int lookupChar(const char* buf, int len, char c) {
  for (int i = 0; i < len; i++) {
    if (c == buf[i]) return i;
  }
  return -1;
}

static int trimRight(const char* src, int len) {
  int i = len - 1;
  for (; i >= 0; i--) {
    if (!isspace(src[i])) return i + 1;
  }
  return 0;
}

static int trimLeft(const char* src, int len) {
  for (int i = 0; i < len; i++) {
    if (!isspace(src[i])) return i;
  }
  return len;
}

struct IniParser {
  // File that being parsed.
  FILE* file;
  // Line cursor
  int cursor;
  // Length of the line.
  int line_len;
  // Current number of the line
  int lineno;

  // @ugly: Flag that indicates that file should be closed with the parser.
  bool file_owned;

  // Line buffer
  char line[INI_MAX_LINE_SIZE];
};

IniParser* iniParserNew(FILE* file) {
  assert(file != NULL);

  IniParser* parser = CAST(IniParser*, malloc(sizeof(IniParser)));
  memset(parser, 0, sizeof(IniParser));

  parser->file = file;
  parser->file_owned = false;

  return parser;
}

IniParser* iniParserOpen(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    return NULL;
  }

  IniParser* parser = CAST(IniParser*, malloc(sizeof(IniParser)));
  memset(parser, 0, sizeof(IniParser));

  parser->file = file;
  parser->file_owned = true;

  return parser;
}

void iniParserFree(IniParser* parser) {
  if (parser != NULL) {
    if (parser->file_owned) {
      fclose(parser->file);
    }
    free(parser);
  }
}

// iniParserConsume reads next line from the file.
static bool iniParserConsume(IniParser* parser) {
  parser->cursor   = 0;
  parser->line_len = 0;

  memset(parser->line, 0, INI_MAX_LINE_SIZE);

  if (fgets(parser->line, INI_MAX_LINE_SIZE, parser->file) == NULL) {
    return false;
  }

  int length       = stringLength(parser->line, INI_MAX_LINE_SIZE);
  parser->line_len = trimRight(parser->line, length);

  // @note: this is mostly done for debug purposes.
  parser->line[parser->line_len] = '\0';

  return true;
}

// iniParserLine returns current line
static const char* iniParserLine(IniParser* parser, int* len) {
  if (parser->line_len == 0) {
    if (len != NULL) {
      *len = parser->line_len;
    }
    return NULL;
  }

  int begin = trimLeft(parser->line + parser->cursor, parser->line_len);

  parser->cursor   += begin;
  parser->line_len -= begin;

  if (len != NULL) {
    *len = parser->line_len;
  }

  if (parser->line_len == 0) {
    return NULL;
  }

  return (parser->line + parser->cursor);
}

int iniParseKey(IniParser* parser, char* dst, int maxlen) {
  // @note: accounting for the '\0' at the end
  maxlen--;

  while (iniParserConsume(parser)) {
    int length;
    const char* line = iniParserLine(parser, &length);

    if (length == 0 || line[0] == ';' || line[0] == '#') {
      continue;
    }

    int separator = lookupChar(line, length, '=');
    if (separator < 2) {
      return INI_ERROR_INVALID_SYNTAX;
    }

    parser->cursor   += separator + 1;
    parser->line_len -= separator + 1;

    int key_len = trimRight(line, separator);
    if (key_len > maxlen) {
      return INI_ERROR_CODE_OVERFLOW;
    }

    // @todo: would be nice to normalize key for the following cases
    // - "key" = ...
    // - key . subkey = ...

    strncpy(dst, line, key_len);
    dst[key_len] = '\0';

    return key_len;
  }

  // EOF
  return INI_ERROR_CODE_NONE;
}

int iniParseValue(IniParser* parser, char* dst, int maxlen) {
  int length = 0;
  const char* line = iniParserLine(parser, &length);

  if (length == 0) {
    return 0;
  }

  // @note: accounting for the '\0' at the end
  maxlen--;

  if (line[length - 1] != '\\') {
    length = (length < maxlen) ? length : maxlen;
    strncpy(dst, line, length);

    dst[length] = '\0';
    return length;
  }

  int cursor = 0;
  bool more  = true;
  while (more && length > 0 && maxlen > 0) {
    more = line[length - 1] == '\\';
    if (more) {
      length = trimRight(line, length - 1);
    }

    length = (length < maxlen) ? length : maxlen;
    strncpy(dst + cursor, line, length);

    cursor += length;
    maxlen -= length;

    if (maxlen > 0) {
      dst[cursor] = ' ';

      cursor++;
      maxlen--;
    }

    if (more && maxlen > 0) {
      iniParserConsume(parser);
      line = iniParserLine(parser, &length);
    }
  }

  dst[cursor] = '\0';

  return cursor;
}

#endif

#endif // INI_PARSE_H
