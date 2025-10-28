# ESP32-C6 REPL Example

This example demonstrates how to integrate V4-REPL into an ESP32-C6 application.

## Overview

Provides a UART-based interactive Forth REPL using:
- **V4 VM**: Executes Forth bytecode
- **V4-front**: Compiles Forth source to bytecode
- **V4-REPL**: Manages the read-eval-print loop

## Features

- ✅ Interactive Forth REPL over UART
- ✅ Persistent word definitions
- ✅ Stack display after each command
- ✅ Detailed error messages with position information
- ✅ Line editing (backspace support)
- ✅ Ctrl+C to clear data stack

## Hardware Requirements

- ESP32-C6 development board
- USB cable for UART connection

## Memory Configuration

Default configuration (can be adjusted in `main.c`):

```c
#define VM_MEMORY_SIZE (16 * 1024)  // 16KB VM RAM
#define ARENA_SIZE (4 * 1024)       // 4KB arena for word names
#define LINE_BUFFER_SIZE 256        // Maximum line length
```

**Total RAM usage**: ~21KB + REPL overhead (~2KB) = **~23KB**

ESP32-C6 has **512KB RAM**, so this leaves plenty of room for your application.

## Integration with V4-ports

### Directory Structure

```
V4-ports/esp32c6/
├── components/
│   ├── v4_hal_esp32c6/       # HAL implementation
│   ├── v4/                    # V4 VM (symlink to V4 repo)
│   ├── v4front/               # V4-front (symlink to V4-front repo)
│   └── v4repl/                # V4-REPL (symlink to V4-repl repo)
└── examples/
    └── v4-repl/
        ├── main/
        │   └── main.c         # Copy from V4-repl/examples/esp32c6/main.c
        └── CMakeLists.txt
```

### Step 1: Create Component Symlinks

```bash
cd V4-ports/esp32c6/components

# Create symlinks to V4, V4-front, and V4-repl
ln -s ../../../../V4 v4
ln -s ../../../../V4-front v4front
ln -s ../../../../V4-repl v4repl
```

### Step 2: Create Example Project

```bash
cd V4-ports/esp32c6/examples
mkdir -p v4-repl/main
cd v4-repl
```

### Step 3: Copy Main Source

```bash
cp ../../../../V4-repl/examples/esp32c6/main.c main/
```

### Step 4: Create Project CMakeLists.txt

Create `V4-ports/esp32c6/examples/v4-repl/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(v4-repl-example)
```

### Step 5: Create Main Component CMakeLists.txt

Create `V4-ports/esp32c6/examples/v4-repl/main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES v4 v4front v4repl v4_hal_esp32c6
)
```

### Step 6: Build and Flash

```bash
cd V4-ports/esp32c6/examples/v4-repl

# Configure for ESP32-C6
idf.py set-target esp32c6

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

## Usage Examples

### Basic Arithmetic

```forth
v4> 2 3 +
 ok [1]: 5

v4> 10 5 - 2 *
 ok [1]: 10
```

### Stack Operations

```forth
v4> 1 2 3
 ok [3]: 1 2 3

v4> drop
 ok [2]: 1 2

v4> swap
 ok [2]: 2 1
```

### Word Definitions

```forth
v4> : square dup * ;
 ok

v4> 5 square
 ok [1]: 25

v4> : cube dup dup * * ;
 ok

v4> 3 cube
 ok [1]: 27
```

### Variables

```forth
v4> variable x
 ok

v4> 42 x !
 ok

v4> x @
 ok [1]: 42
```

### Error Handling

```forth
v4> undefined_word
Error: Unknown word: undefined_word
  undefined_word
  ^

v4> 5 0 /
Error: Division by zero
```

## HAL Requirements

The example requires the following HAL functions (implemented in `v4_hal_esp32c6`):

```c
/**
 * @brief Read a character from UART (non-blocking)
 * @return 0 on success, -1 if no data available
 */
int v4_hal_uart_getc(int port, char *c);

/**
 * @brief Write a character to UART
 * @return 0 on success, negative on error
 */
int v4_hal_uart_putc(int port, char c);

/**
 * @brief Delay for specified milliseconds
 */
void v4_hal_delay_ms(int ms);
```

## Customization

### Adjusting Memory Limits

Edit memory configuration in `main.c`:

```c
#define VM_MEMORY_SIZE (32 * 1024)  // Increase to 32KB
#define ARENA_SIZE (8 * 1024)       // Increase to 8KB
#define LINE_BUFFER_SIZE 512        // Increase line buffer
```

### Adding Custom Words

You can pre-register custom native words before entering the REPL loop:

```c
/* Example: Add a custom "led-on" word */
static const uint8_t led_on_code[] = {
    /* Bytecode to turn on LED */
};

vm_register_word(g_vm, "led-on", led_on_code, sizeof(led_on_code));
v4front_context_register_word(g_compiler_ctx, "led-on", word_id);
```

### Custom Prompt

Modify `print_prompt()` function in `main.c`:

```c
static void print_prompt(void) {
  printf("forth> ");  // Custom prompt
}
```

## Troubleshooting

### Build Errors

**Error**: `v4repl/repl.h: No such file or directory`

**Solution**: Ensure V4-repl component symlink exists:
```bash
cd V4-ports/esp32c6/components
ln -s ../../../../V4-repl v4repl
```

**Error**: `undefined reference to v4_hal_uart_getc`

**Solution**: Add `v4_hal_esp32c6` to `REQUIRES` in main component CMakeLists.txt.

### Runtime Issues

**Issue**: No UART output

**Solution**: Check UART configuration in `v4_hal_esp32c6`. Default is UART0 at 115200 baud.

**Issue**: Stack overflow / crashes

**Solution**: Increase ESP-IDF main task stack size in `sdkconfig`:
```
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
```

**Issue**: Out of memory errors

**Solution**: Reduce `VM_MEMORY_SIZE` or `ARENA_SIZE` in `main.c`.

## Performance Notes

- **Compilation**: ~1-5ms for simple expressions
- **Execution**: ~100-500 cycles per word (depends on complexity)
- **Memory**: ~23KB total RAM usage with default config
- **Flash**: ~50KB code size (with -Os optimization)

## License

This example is dual-licensed under MIT and Apache-2.0, same as V4-REPL.
