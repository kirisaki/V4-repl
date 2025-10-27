# V4 REPL

Interactive Forth REPL for the V4 virtual machine.

## Features

- Interactive command-line interface with linenoise
- **Persistent word definitions across lines** - define words on one line, use them on subsequent lines
- **Stack preservation** - stack contents are maintained across all operations
- **Detailed error messages** - error position information with visual indicators
- Stack display after each command
- Command history (when filesystem support is enabled)
- Compile-time configurable for embedded systems

## Requirements

- C++17 compiler
- CMake 3.15 or later
- V4 VM library
- V4-front compiler library
- linenoise (fetched automatically)

## Building

### Quick start with Makefile

```bash
make          # Build with local V4/V4-front
make run      # Build and run REPL
make test     # Run smoke tests
make help     # Show all available targets
```

### Standard build with CMake

```bash
mkdir build
cd build
cmake ..
make
```

### Embedded build without filesystem

For systems without filesystem support (no history feature):

```bash
# Using Makefile
make build-no-fs

# Or using CMake
mkdir build
cd build
cmake -DWITH_FILESYSTEM=OFF ..
make
```

### Custom V4/V4-front paths

By default, the build system looks for V4 and V4-front in `../V4` and `../V4-front`.

```bash
# Using Makefile
make build V4_PATH=/path/to/V4 V4FRONT_PATH=/path/to/V4-front

# Or using CMake
cmake -DV4_LOCAL_PATH=/path/to/V4 \
      -DV4FRONT_LOCAL_PATH=/path/to/V4-front \
      ..
```

### Available Makefile targets

```bash
make              # Build (default)
make build        # Build with local dependencies
make build-no-fs  # Build without filesystem support
make release      # Release build
make run          # Build and run REPL
make test         # Run smoke tests
make size         # Quick binary size check
make size-report  # Detailed size analysis with recommendations
make clean        # Clean build artifacts
make format       # Format code
make asan         # Build with AddressSanitizer
make ubsan        # Build with UndefinedBehaviorSanitizer
make help         # Show all targets
```

## Usage

```bash
$ ./v4-repl
V4 REPL v0.1.0
Type 'bye' or press Ctrl+D to exit

v4> 1 2 +
 ok [1]: 3

v4> DUP *
 ok [1]: 9

v4> : SQUARE DUP * ;
 ok

v4> 5 SQUARE
 ok [1]: 25

v4> : DOUBLE DUP + ;
 ok [1]: 25

v4> 3 DOUBLE
 ok [2]: 25 6

v4> bye
Goodbye!
```

## Stack Display Format

After each successful command, the stack is displayed as:

```
 ok [depth]: val1 val2 ... valN
```

Where:
- `depth` is the number of elements on the stack
- Values are shown from bottom to top

If the stack is empty:

```
 ok
```

## Commands

- `bye` or `quit` - Exit the REPL
- Ctrl+D - Exit the REPL
- Any valid V4 Forth code

## Features in Detail

This version includes stateful compilation and detailed error reporting:

### Persistent Word Definitions

Words can now be defined on one line and used on subsequent lines:

```forth
v4> : SQUARE DUP * ;
 ok

v4> 5 SQUARE
 ok [1]: 25

v4> 7 SQUARE
 ok [1]: 49
```

### Stack Preservation

The data stack is preserved across all operations, including word definitions:

```forth
v4> 10 20
 ok [2]: 10 20

v4> : DOUBLE DUP + ;
 ok [2]: 10 20        # Stack preserved!

v4> 30 DOUBLE
 ok [3]: 10 20 60
```

### Detailed Error Messages

Errors now show the exact position with visual indicators:

```forth
v4> 1 2 UNKNOWN +
Error: unknown token at line 1, column 5
  1 2 UNKNOWN +
      ^~~~~~~
```

## History

When compiled with filesystem support (default), command history is saved to `~/.v4_history`.

Features:
- Up/Down arrows to navigate history
- Maximum 1000 entries
- Persists across sessions

## Compiler Flags

The project is compiled with:
- `-fno-exceptions` - No exception handling
- `-fno-rtti` - No runtime type information

This makes it suitable for embedded systems and resource-constrained environments.

## Binary Size

v4-repl is designed to be compact and suitable for embedded systems:

| Build Type | Size | Use Case |
|------------|------|----------|
| Debug | 143KB | Development (includes debug symbols) |
| Debug (stripped) | 59KB | Testing without debugger |
| Release (stripped) | **67KB** | **Production (recommended)** |
| MinSizeRel (stripped) | ~60KB | Size-critical embedded systems |

Check binary size:
```bash
make size         # Quick check
make size-report  # Detailed analysis with recommendations
```

The compact size makes v4-repl ideal for:
- Embedded devices (ESP32, STM32, etc.)
- IoT applications
- Resource-constrained environments
- Quick deployment without large dependencies

## Project Structure

```
V4-repl/
├── CMakeLists.txt          # Build configuration
├── Makefile                # Convenient build targets
├── README.md               # This file
├── .gitignore
├── src/
│   ├── main.cpp            # Entry point
│   ├── repl.hpp            # REPL class interface
│   └── repl.cpp            # REPL implementation
├── test_smoke.sh           # Smoke test script
├── size_report.sh          # Binary size analysis script
└── build/                  # Build directory (generated)
```

## License

Same as V4 project.
