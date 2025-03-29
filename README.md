# Flag - Command-Line Argument Parsing

A minimalist, header-only library for parsing command-line arguments in C and C++, inspired by:
- [peterbourgon/ff](https://github.com/peterbourgon/ff)
- [tsoding/flag.h](https://github.com/tsoding/flag.h)

## Installation

As a header-only library, simply copy `flag.h` into your project's include path
or directly into your source directory.

## Usage

1. Add `#include "flag.h"` to your source file.
2. In exactly *one* C or C++ file in your project, define `FLAGS_IMPLEMENTATION` *before* including the header. This creates the necessary function implementations.
```c
#define FLAGS_IMPLEMENTATION
#include "flag.h"
```

## Example

```c
#define FLAGS_IMPLEMENTATION // Define implementation in one file
#include "flag.h"

#include <stdio.h> // For printf, stderr
#include <time.h>  // For time_t, strftime, localtime

int main(int argc, char** argv) { // envv optional if library uses it
  char buf[128] = {0};

  // Variables to hold flag values
  bool   boolean  = false;
  char*  str      = NULL;
  int    integer  = 0;
  float  floating = 0.0f;
  time_t time     = 0;

  // Define flags
  // flagBoolVar(pointer, long_name, short_name, description)
  flagBoolVar(&boolean, "bool", 'b', "Boolean flag (default: false)");
  // flagStringVar(pointer, long_name, short_name, default_value, description)
  flagStringVar(&str, "str", 's', "default_value", "String flag");
  // flagIntVar(pointer, long_name, short_name, default_value, description)
  flagIntVar(&integer, "int", 'i', 42, "Integer flag");
  // flagFloatVar(pointer, long_name, short_name, default_value, description)
  flagFloatVar(&floating, "float", 'f', 3.14f, "Floating point flag");
  // flagTimeVar(pointer, long_name, short_name, default_value (epoch), description)
  // Note: FLAGS_TIME_FMT is defined by the library.
  flagTimeVar(&time, "time", 't', 0, "Time flag (YYYY-MM-DDTHH:MM:SS)");

  // Parse command line arguments
  if (!flagParse(argc, argv)) {
    // Print usage information or errors if parsing failed
    flagPrintError(stderr);
    return 1;
  }

  strftime(buf, sizeof(buf), FLAGS_TIME_FMT, localtime(&time));

  printf("--- Parsed Flag Values ---\n");
  printf("bool  = %s\n", boolean ? "true": "false");
  printf("str   = %s\n", str);
  printf("int   = %d\n", integer);
  printf("float = %f\n", floating);
  printf("time  = %s\n", buf);

  return 0;
}
```
