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

#ifndef FLAGS_H
#define FLAGS_H

#include <time.h>
#include <stdio.h>
#include <stdbool.h>

// Maximum number of flags that could be registered.
#ifndef FLAGS_MAX
#define FLAGS_MAX 256
#endif

// Maximum number of characters in the flag or env string.
#ifndef FLAGS_FLAG_MAX_LEN
#define FLAGS_FLAG_MAX_LEN 64
#endif

// Library format for time
#ifndef FLAGS_TIME_FMT
#define FLAGS_TIME_FMT "%Y-%m-%dT%H:%M:%S"
#endif

typedef struct FlagSet FlagSet;

#ifdef __cplusplus
extern "C" {
#endif

// flagPrintUsage prints usage.
void flagPrintUsage(FILE* stream);
// flagIgnoreUnknown allows changing the parser's behavior when an unknown flag is encountered.
void flagIgnoreUnknown(bool ignore);
// flagBoolVar adds boolean flag to the default flag set.
void flagBoolVar(bool* dst, char* name, char short_name, char* description);
// flagStringVar adds string flag to the default flag set.
void flagStringVar(char** dst, char* name, char short_name, char* default_value, char* description);
// flagIntVar adds int flag to the default flag set.
void flagIntVar(int* dst, char* name, char short_name, int default_value, char* description);
// flagFloatVar adds float flag to the default flag set.
void flagFloatVar(float* dst, char* name, char short_name, float default_value, char* description);
// flagDoubleVar adds double flag to the default flag set.
void flagDoubleVar(double* dst, char* name, char short_name, double default_value, char* description);
// flagTimeVar adds time_t flag to the default flag set.
void flagTimeVar(time_t* dst, char* name, char short_name, time_t default_value, char* description);
// flagParse attempts to parse flags from command line arguments to the default flag set.
// NOTE: repeated call to the flagParse may result in unpredicted results.
bool flagParse(int argc, char** argv);
// flagPrintError prints error if any present and exits with code 1
void flagPrintError(FILE* stream);

// flagSetNew returns new flag set.
FlagSet* flagSetNew(void);
// flagSetFree frees resources allocated by the flag set.
void flagSetFree(FlagSet* fs);
// flagSetPrintUsage prints usage.
void flagSetPrintUsage(FlagSet* fs, FILE* stream);
// flagSetIgnoreUnknown allows changing the parser's behavior when an unknown flag is encountered.
void flagSetIgnoreUnknown(FlagSet* fs, bool ignore);
// flagSetBoolVar adds boolean flag to the flag set.
void flagSetBoolVar(FlagSet* fs, bool* dst,
    char* name, char short_name, char* description);
// flagSetStringVar adds string flag to the flag set.
void flagSetStringVar(FlagSet* fs, char** dst,
    char* name, char short_name, char* default_value, char* description);
// flagSetIntVar adds int flag to the flag set.
void flagSetIntVar(FlagSet* fs, int* dst,
    char* name, char short_name, int default_value, char* description);
// flagSetFloatVar adds float flag to the flag set.
void flagSetFloatVar(FlagSet* fs, float* dst,
    char* name, char short_name, float default_value, char* description);
// flagSetDoubleVar adds double flag to the flag set.
void flagSetDoubleVar(FlagSet* fs, double* dst,
    char* name, char short_name, double default_value, char* description);
// flagSetTimeVar adds time_t flag to the default flag set.
void flagSetTimeVar(FlagSet* fs, time_t* dst,
    char* name, char short_name, time_t default_value, char* description);
// flagSetParse attempts to parse flags from command line arguments.
// NOTE: repeated call to the flagParse may result in unpredicted results.
bool flagSetParse(FlagSet* fs, int argc, char** argv);
// flagSetPrintError prints error if any present and exits with code 1
void flagSetPrintError(FlagSet* fs, FILE* stream);

#ifdef WITH_INI
// flagConfig adds flag for the configuration file.
void flagConfig(char* name, char short_name, char* description);
// flagSetConfig adds flag for the config.
void flagSetConfig(FlagSet* fs, char* name, char short_name, char* description);
#endif

#ifdef __cplusplus
}
#endif

#ifdef FLAGS_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


#ifdef __cplusplus
# define CAST(type, v) static_cast<type>(v)
#else
# define CAST(type, v) (type)(v)
#endif


// @todo: add more types 
typedef enum { 
  FLAG_TYPE_BOOL = 0,
  FLAG_TYPE_STRING,
  FLAG_TYPE_INT,
  FLAG_TYPE_FLOAT,
  FLAG_TYPE_DOUBLE,
  FLAG_TYPE_TIME,
} FlagType;

// Union type that will store flag value
typedef union {
  // FLAG_TYPE_STRING
  char* as_string;
  // FLAG_TYPE_INT
  int as_int;
  // FLAG_TYPE_FLOAT
  float as_float;
  // FLAG_TYPE_FLOAT
  double as_double;
  // FLAG_TYPE_TIME
  time_t as_time_t;
} FlagValue;

// Flag contains information about singular flag.
typedef struct {
  // Type of flag
  FlagType type;
  // Flag name
  char* name;
  // Short name of flag 0 - means no short name.
  char short_name;
  // Flag description
  char* description;
  // Value pointer
  void* ptr;
  // Default value of the flag
  FlagValue default_value;
} Flag;

typedef enum {
  // No error
  FLAG_ERROR_CODE_NONE = 0,
  // Error that occurs when help flag is passed
  FLAG_ERROR_CODE_HELP,
  // Unknown flag
  FLAG_ERROR_CODE_UNKNOWN,
  // Missing flag value
  FLAG_ERROR_CODE_MISSING_VALUE,
  // Flag value is invalid
  FLAG_ERROR_CODE_INVALID_VALUE,
  // Failed to open config file
  FLAG_ERROR_CODE_OPEN_CONFIG_FILE,
} FlagErrorCode;

// FlagSet contains list of registered flags.
struct FlagSet {
  // Number of flags
  int flags_len;
  // Error code
  FlagErrorCode error_code;

#ifdef WITH_INI
  // Name for the config flag
  char* config_flag_name;
  // Config flag description
  char* config_flag_desc;
  // Short name for the config flag
  char config_flag_short_name;
#endif

  // Should we ignore unknown flags?
  bool ignore_unknown;

  // Name of the flag where error occurred
  char error_flag_name[FLAGS_FLAG_MAX_LEN];
  // Registered flags.
  Flag flags[FLAGS_MAX];
};

FlagSet* flagSetNew(void) {
  FlagSet* fs = (FlagSet*)malloc(sizeof(FlagSet));
  memset(fs, 0, sizeof(FlagSet));
  return fs;
}

void flagSetFree(FlagSet* fs) {
  free(fs);
}

void flagSetPrintUsage(FlagSet* fs, FILE* stream) {
  static char buf[512] = { 0 };

  Flag* flag;
  // length of the current flag name
  int len          = 0;
#ifdef WITH_INI
  // max length of the flag name
  int max_flag_len = fs->config_flag_name != NULL ? strlen(fs->config_flag_name) : 0;
#else
  int max_flag_len = 0;
#endif

  for (int i = 0; i < fs->flags_len; i++) {
    flag = fs->flags + i;
    if ((len = strlen(flag->name)) > max_flag_len) {
      max_flag_len = len;
    }
  }

  max_flag_len += 5;

  fprintf(stream, "FLAGS\n");

#ifdef WITH_INI
  if (fs->config_flag_name != NULL) {
    if (fs->config_flag_short_name != 0) {
      fprintf(stream, "  -%c, ", fs->config_flag_short_name);
    } else {
      fprintf(stream, "      ");
    }

    fprintf(stream, "--%-*s %s\n", max_flag_len,
        fs->config_flag_name, fs->config_flag_desc);
  }
#endif

  for (int i = 0; i < fs->flags_len; i++) {
    flag = fs->flags + i;
    if (flag->short_name != 0) {
      fprintf(stream, "  -%c, ", flag->short_name);
    } else {
      fprintf(stream, "      ");
    }

    fprintf(stream, "--%-*s %s", max_flag_len, flag->name, flag->description);
    switch (flag->type) {
    case FLAG_TYPE_BOOL:
      break;
    case FLAG_TYPE_STRING:
      {
        if (strlen(flag->default_value.as_string)) {
          fprintf(stream, " (default: %s)", flag->default_value.as_string);
        }
      } break;
    case FLAG_TYPE_INT:
      {
        fprintf(stream, " (default: %d)", flag->default_value.as_int);
      } break;
    case FLAG_TYPE_FLOAT:
      {
        fprintf(stream, " (default: %f)", flag->default_value.as_float);
      } break;
    case FLAG_TYPE_DOUBLE:
      {
        fprintf(stream, " (default: %f)", flag->default_value.as_double);
      } break;
    case FLAG_TYPE_TIME:
      {
        if (flag->default_value.as_time_t != 0) {
          memset(buf, 0, 512);
          strftime(buf, 512, FLAGS_TIME_FMT, localtime(&flag->default_value.as_time_t));
          fprintf(stream, " (default: %s)", buf);
        }
      } break;
    }
    fprintf(stream, "\n");
  }
  fprintf(stream, "  -h, --%-*s Show this help message\n", max_flag_len, "help");

  fprintf(stream, "\n");
}

void flagSetIgnoreUnknown(FlagSet* fs, bool ignore) {
  fs->ignore_unknown = ignore;
}


static Flag* flagMake(FlagSet* fs, void* dst, FlagType type, 
    char* name, char short_name, char* description) {
  assert(fs->flags_len < FLAGS_MAX);
  Flag* flag = fs->flags + fs->flags_len;

  flag->type        = type;
  flag->name        = name;
  flag->short_name  = short_name;
  flag->description = description;
  flag->ptr         = dst;

  fs->flags_len++;
  return flag;
}

void flagSetBoolVar(FlagSet* fs, bool* dst,
    char* name, char short_name, char* description) {
  flagMake(fs, dst, FLAG_TYPE_BOOL, name, short_name, description);
  *dst = false;
}

void flagSetStringVar(FlagSet* fs, char** dst,
    char* name, char short_name, char* default_value, char* description) {
  Flag* flag = flagMake(fs, dst, FLAG_TYPE_STRING, name, short_name, description);

  flag->default_value.as_string = default_value;
  *dst = default_value;
}

void flagSetIntVar(FlagSet* fs, int* dst,
    char* name, char short_name, int default_value, char* description) {
  Flag* flag = flagMake(fs, dst, FLAG_TYPE_INT, name, short_name, description);

  flag->default_value.as_int = default_value;
  *dst = default_value;
}

void flagSetFloatVar(FlagSet* fs, float* dst,
    char* name, char short_name, float default_value, char* description) {
  Flag* flag = flagMake(fs, dst, FLAG_TYPE_FLOAT, name, short_name, description);

  flag->default_value.as_float = default_value;
  *dst = default_value;
}

void flagSetDoubleVar(FlagSet* fs, double* dst,
    char* name, char short_name, double default_value, char* description) {
  Flag* flag = flagMake(fs, dst, FLAG_TYPE_DOUBLE, name, short_name, description);

  flag->default_value.as_double = default_value;
  *dst = default_value;
}

void flagSetTimeVar(FlagSet* fs, time_t* dst,
    char* name, char short_name, time_t default_value, char* description) {
  Flag* flag = flagMake(fs, dst, FLAG_TYPE_TIME, name, short_name, description);

  flag->default_value.as_time_t = default_value;
  *dst = default_value;
}

static char* stringDuplicate(const char* src, int maxlen) {
  if (src == NULL) {
    return NULL;
  }

  int len = strlen(src);
  len     = (len < maxlen) ? len : maxlen;

  char* result = CAST(char*, malloc(len));
  memcpy(result, src, len);
  result[len] = 0;

  return result;
}

// shiftArgs shifts arguments by one and returns current argument.
static char* shiftArgs(int* argc, char*** argv) {
  assert(*argc > 0);

  char* value = **argv;
  *argv += 1;
  *argc -= 1;

  return value;
}

static void setError(FlagSet* fs, FlagErrorCode code, char* flag_name) {
  fs->error_code = code;
  strncpy(fs->error_flag_name, flag_name, FLAGS_FLAG_MAX_LEN);
}


// lookupConfigFlag attempts to find the flag configuration by its name.
// Returns true if the configuration is found and sets `dst` to pointer to the
// found configuration; otherwise, returns false.
static bool lookupConfigFlag(FlagSet* fs, Flag** dst, char* flag) {
  int len = strlen(flag);
  if (len < 1) {
    return false;
  }

  Flag* item;
  for (int i = 0; i < fs->flags_len; i++) {
    item = fs->flags + i;
    if (strncmp(item->name, flag, len) == 0) {
      *dst = item;
      return true;
    }
  }

  return false;
}

// lookupFlag attempts to find the flag configuration by its name.
// Returns true if the configuration is found and sets `dst` to pointer to the
// found configuration; otherwise, returns false.
static bool lookupFlag(FlagSet* fs, Flag** dst, char* flag) {
  int len = strlen(flag);
  if (len < 2 || flag[0] != '-') {
    return false;
  }

  Flag* item;
  if (flag[1] == '-') {
    // Long name is used
    char* name = flag + 2;
    for (int i = 0; i < fs->flags_len; i++) {
      item = fs->flags + i;
      if (strncmp(item->name, name, len - 2) == 0) {
        *dst = item;
        return true;
      }
    }
  } else if (len == 2) {
    // Short name is used
    char name = flag[1];
    for (int i = 0; i < fs->flags_len; i++) {
      item = fs->flags + i;
      if (name == item->short_name) {
        *dst = item;
        return true;
      }
    }
  }

  return false;
}

static bool isHelpFlag(char* flag) {
  int len = strlen(flag);
  if (len < 2 || flag[0] != '-') {
    return false;
  }

  if (flag[1] == '-') {
    return strcmp("help", flag + 2) == 0;
  } else if (len == 2) {
    return flag[1] == 'h';
  }

  return false;
}

#ifdef WITH_INI

// isConfigFlag checks if the flag is the one that specifies a configuration file.
static bool isConfigFlag(FlagSet* fs, char* flag);  

// parseIniConfig attempts to populate flags from an INI configuration file.
// Returns true on success. Returns false on error and populates
// error_code and error_flag_name fields in the FlagSet structure.
static bool parseIniConfig(FlagSet* fs, const char* filename);

#endif

bool flagSetParse(FlagSet* fs, int argc, char** argv) {
  shiftArgs(&argc, &argv);

  while (argc > 0) {
    Flag* conf;
    char* flag = shiftArgs(&argc, &argv);
#ifdef WITH_INI
    if (isConfigFlag(fs, flag)) {
      if (argc == 0) {
        setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, flag);
        return false;
      }

      char* filename = shiftArgs(&argc, &argv);
      if (!parseIniConfig(fs, filename)) {
        return false;
      }

      continue;
    }
#endif

    if (!lookupFlag(fs, &conf, flag)) {
      if (fs->ignore_unknown) {
        continue;
      }

      if (isHelpFlag(flag)) {
        setError(fs, FLAG_ERROR_CODE_HELP, flag);
        return false;
      }

      setError(fs, FLAG_ERROR_CODE_UNKNOWN, flag);
      return false;
    }

    switch (conf->type) {
      case FLAG_TYPE_BOOL:
        {
          *((bool*)conf->ptr) = true;
        } break;
      case FLAG_TYPE_STRING: 
        {
          if (argc == 0) {
            setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, conf->name);
            return false;
          }
          char* value = shiftArgs(&argc, &argv);
          *((char**)conf->ptr) = value;
        } break;
      case FLAG_TYPE_INT:
        {
          if (argc == 0) {
            setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, conf->name);
            return false;
          }
          char* end;
          char* value = shiftArgs(&argc, &argv);
          long result = strtol(value, &end, 10);

          if (end == value || result < INT_MIN || INT_MAX < result) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);
            return false;
          }

          *((int*)conf->ptr) = (int)result;
        } break;
      case FLAG_TYPE_FLOAT:
        {
          if (argc == 0) {
            setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, conf->name);
            return false;
          }
          char* end;
          char* value = shiftArgs(&argc, &argv);
          float result = strtof(value, &end);

          if (end == value) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);
            return false;
          }

          *((float*)conf->ptr) = (float)result;
        } break;
      case FLAG_TYPE_DOUBLE:
        {
          if (argc == 0) {
            setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, conf->name);
            return false;
          }
          char* end;
          char* value = shiftArgs(&argc, &argv);
          float result = strtod(value, &end);

          if (end == value) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);
            return false;
          }

          *((double*)conf->ptr) = (double)result;
        } break;
      case FLAG_TYPE_TIME:
        {
          if (argc == 0) {
            setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, conf->name);
            return false;
          }
          char* value = shiftArgs(&argc, &argv);

          struct tm result = { 0 };
          if (strptime(value, FLAGS_TIME_FMT, &result) == value) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);
            return false;
          }

          *((time_t*)conf->ptr) = mktime(&result);
        } break;
    }
  }

  return true;
}

void flagSetPrintError(FlagSet* fs, FILE* stream) {
  switch (fs->error_code) {
    case FLAG_ERROR_CODE_UNKNOWN:
      fprintf(stream, "ERROR: unknown flag \"%s\"\n\n", fs->error_flag_name);
      break;
    case FLAG_ERROR_CODE_MISSING_VALUE:
      fprintf(stream, "ERROR: missing value for flag \"%s\"\n\n", fs->error_flag_name);
      break;
    case FLAG_ERROR_CODE_INVALID_VALUE:
      fprintf(stream, "ERROR: invalid value for flag \"%s\"\n\n", fs->error_flag_name);
      break;
    case FLAG_ERROR_CODE_HELP:
      flagSetPrintUsage(fs, stream);
      exit(0);
      break;
    default:
      // Nothing to do
      return;
      break;
  }

  flagSetPrintUsage(fs, stream);

  exit(1);
}

// Default flag set is a global flag set that will be used by the library  
// functions that do not accept FlagSet as the first argument.
static FlagSet global_flag_set;

void flagPrintUsage(FILE* stream) {
  flagSetPrintUsage(&global_flag_set, stream);
}

void flagIgnoreUnknown(bool ignore) {
  flagSetIgnoreUnknown(&global_flag_set, ignore);
}


void flagBoolVar(bool* dst,
    char* name, char short_name, char* description) {
  flagSetBoolVar(&global_flag_set, dst, name, short_name, description);
}

void flagStringVar(char** dst,
    char* name, char short_name, char* default_value, char* description) {
  flagSetStringVar(&global_flag_set, dst, name, short_name, default_value, description);
}

void flagIntVar(int* dst,
    char* name, char short_name, int default_value, char* description) {
  flagSetIntVar(&global_flag_set, dst, name, short_name, default_value, description);
}

void flagFloatVar(float* dst,
    char* name, char short_name, float default_value, char* description) {
  flagSetFloatVar(&global_flag_set, dst, name, short_name, default_value, description);
}

void flagDoubleVar(double* dst,
    char* name, char short_name, double default_value, char* description) {
  flagSetDoubleVar(&global_flag_set, dst, name, short_name, default_value, description);
}

void flagTimeVar(time_t* dst,
    char* name, char short_name, time_t default_value, char* description) {
  flagSetTimeVar(&global_flag_set, dst, name, short_name, default_value, description);
}

bool flagParse(int argc, char** argv) {
  return flagSetParse(&global_flag_set, argc, argv);
}

void flagPrintError(FILE* stream) {
  flagSetPrintError(&global_flag_set, stream);
}

#ifdef WITH_INI

#define INI_IMPLEMENTATION
#include "ini.h"

void flagConfig(char* name, char short_name, char* description) {
  flagSetConfig(&global_flag_set, name, short_name, description);
}

static bool isConfigFlag(FlagSet* fs, char* flag) {
  if (fs->config_flag_name == NULL) {
    return false;
  }

  int len = strlen(flag);
  if (len < 2 || flag[0] != '-') {
    return false;
  }

  if (flag[1] == '-') {
    // Long name is used
    char* name = flag + 2;
    return (strncmp(fs->config_flag_name, name, len - 2) == 0);
  } else if (len == 2) {
    return (fs->config_flag_short_name != '\0' &&
        flag[1] == fs->config_flag_short_name);
  }

  return false;
}

void flagSetConfig(FlagSet* fs, char* name, char short_name, char* description) {
  fs->config_flag_name       = name;
  fs->config_flag_short_name = short_name;
  fs->config_flag_desc       = description;
}

#define CONFIG_BUFFER_SIZE 512

static bool parseIniConfig(FlagSet* fs, const char* filename) {
  char buf[CONFIG_BUFFER_SIZE] = {0};

  IniParser* parser = iniParserOpen(filename);
  if (parser == NULL) {
    setError(fs, FLAG_ERROR_CODE_OPEN_CONFIG_FILE, fs->config_flag_name);
    return false;
  }

  while (iniParseKey(parser, buf, FLAGS_FLAG_MAX_LEN) > 0) {
    Flag* conf;

    if (fs->config_flag_name && strcmp(fs->config_flag_name, buf) == 0) {
      if (!iniParseValue(parser, buf, CONFIG_BUFFER_SIZE)) {
        setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, buf);

        iniParserFree(parser);
        return false;
      }

      if (!parseIniConfig(fs, buf)) {
        return false;
      }

      memset(buf, 0, CONFIG_BUFFER_SIZE);
      continue;
    }

    if (!lookupConfigFlag(fs, &conf, buf)) {
      if (fs->ignore_unknown) {
        memset(buf, 0, CONFIG_BUFFER_SIZE);
        continue;
      }

      setError(fs, FLAG_ERROR_CODE_UNKNOWN, buf);

      iniParserFree(parser);
      return false;
    }

    memset(buf, 0, CONFIG_BUFFER_SIZE);
    int value_len = 0;

    if ((value_len = iniParseValue(parser, buf, CONFIG_BUFFER_SIZE)) == 0) {
      setError(fs, FLAG_ERROR_CODE_MISSING_VALUE, conf->name);

      iniParserFree(parser);
      return false;
    }

    switch (conf->type) {
      case FLAG_TYPE_BOOL:
        {
          if (strncmp(buf, "true", value_len) == 0) {
            *((bool*)conf->ptr) = true;
          } else if (strncmp(buf, "false", value_len) == 0) {
            *((bool*)conf->ptr) = false;
          } else {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);

            iniParserFree(parser);
            return false;
          }
        } break;
      case FLAG_TYPE_STRING: 
        {
          *((char**)conf->ptr) = stringDuplicate(buf, value_len);
        } break;
      case FLAG_TYPE_INT:
        {
          char* end;
          long result = strtol(buf, &end, 10);

          if (end == buf || result < INT_MIN || INT_MAX < result) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);

            iniParserFree(parser);
            return false;
          }

          *((int*)conf->ptr) = (int)result;
        } break;
      case FLAG_TYPE_FLOAT:
        {
          char* end;
          float result = strtof(buf, &end);

          if (end == buf) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);

            iniParserFree(parser);
            return false;
          }

          *((float*)conf->ptr) = (float)result;
        } break;
      case FLAG_TYPE_DOUBLE:
        {
          char* end;
          float result = strtod(buf, &end);

          if (end == buf) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);

            iniParserFree(parser);
            return false;
          }

          *((double*)conf->ptr) = (double)result;
        } break;
      case FLAG_TYPE_TIME:
        {
          struct tm result = { 0 };
          if (strptime(buf, FLAGS_TIME_FMT, &result) == buf) {
            setError(fs, FLAG_ERROR_CODE_INVALID_VALUE, conf->name);

            iniParserFree(parser);
            return false;
          }

          *((time_t*)conf->ptr) = mktime(&result);
        } break;
    }

    memset(buf, 0, CONFIG_BUFFER_SIZE);
  }

  return true;
}

#endif // WITH_INI
#endif // FLAGS_IMPLEMENTATION
#endif // FLAGS_H
