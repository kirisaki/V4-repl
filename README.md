# V4 REPL

Interactive Forth REPL for the V4 virtual machine.

## Features

- Interactive command-line interface with linenoise
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

v4> .
9 ok

v4> : SQUARE DUP * ; 5 SQUARE
 ok [1]: 25

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

## Limitations (Phase 1)

### Word definitions must be on the same line as their usage

Due to the stateless nature of V4-front's compiler, word definitions are only available within the same compilation unit (i.e., the same input line).

**This works:**
```forth
v4> : SQUARE DUP * ; 5 SQUARE
 ok [1]: 25
```

**This does NOT work:**
```forth
v4> : SQUARE DUP * ;
 ok
v4> 5 SQUARE
Error [-1]: unknown token
```

### Word definitions clear the stack

When a line contains word definitions (`:` ... `;`), the VM is reset to ensure correct word indexing. This clears the data stack as a side effect.

```forth
v4> 1 2 +
 ok [1]: 3

v4> 10 20 +
 ok [2]: 3 30        # Stack is preserved

v4> : DOUBLE DUP + ; 5 DOUBLE
 ok [1]: 10          # Stack was cleared due to word definition
```

**Workaround:** Use upcoming Phase 2 features for persistent word definitions, or design calculations to complete within single lines.

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

## Project Structure

```
V4-repl/
├── CMakeLists.txt      # Build configuration
├── README.md           # This file
├── .gitignore
├── src/
│   ├── main.cpp        # Entry point
│   ├── repl.hpp        # REPL class interface
│   └── repl.cpp        # REPL implementation
└── build/              # Build directory (generated)
```

## License

Same as V4 project.
