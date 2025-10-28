/**
 * @file main.c
 * @brief ESP32-C6 REPL Example
 *
 * Demonstrates V4-REPL integration on ESP32-C6 platform.
 * Provides a UART-based Forth REPL using V4 VM and V4-front compiler.
 *
 * Hardware Requirements:
 * - ESP32-C6 development board
 * - USB connection for UART0
 *
 * Features:
 * - Interactive Forth REPL over UART
 * - Persistent word definitions
 * - Stack display after each command
 * - Error reporting with position information
 *
 * Build:
 *   cd V4-ports/esp32c6
 *   idf.py build flash monitor
 */

#include <stdio.h>
#include <string.h>

#include "v4/vm_api.h"
#include "v4front/compile.h"
#include "v4repl/repl.h"

/* Platform HAL - implement these functions for your platform */
#ifndef V4_HAL_H
/* Simple UART interface abstraction */
extern int v4_hal_uart_getc(int port, char *c);
extern int v4_hal_uart_putc(int port, char c);
extern void v4_hal_delay_ms(int ms);
#endif

/* Memory configuration */
#define VM_MEMORY_SIZE (16 * 1024)  /* 16KB VM RAM */
#define ARENA_SIZE (4 * 1024)       /* 4KB arena for word names */
#define LINE_BUFFER_SIZE 256        /* Maximum line length */

/* UART configuration */
#define UART_PORT 0

/* Global state */
static uint8_t g_vm_memory[VM_MEMORY_SIZE];
static uint8_t g_arena_buffer[ARENA_SIZE];
static V4Arena g_arena;
static struct Vm *g_vm = NULL;
static V4FrontContext *g_compiler_ctx = NULL;
static V4ReplContext *g_repl = NULL;

/* Line input buffer */
static char g_line_buffer[LINE_BUFFER_SIZE];
static int g_line_pos = 0;

/**
 * @brief Initialize VM with arena allocator
 *
 * Creates a VM instance with specified memory and arena configuration.
 *
 * @return 0 on success, -1 on failure
 */
static int init_vm(void) {
  /* Initialize arena allocator */
  v4_arena_init(&g_arena, g_arena_buffer, sizeof(g_arena_buffer));

  /* Configure VM */
  VmConfig config = {
      .mem = g_vm_memory,
      .mem_size = sizeof(g_vm_memory),
      .mmio = NULL,
      .mmio_count = 0,
      .arena = &g_arena,
  };

  /* Create VM instance */
  g_vm = vm_create(&config);
  if (!g_vm) {
    printf("ERROR: Failed to create VM\n");
    return -1;
  }

  printf("VM initialized: %d bytes RAM, %d bytes arena\n", VM_MEMORY_SIZE, ARENA_SIZE);
  return 0;
}

/**
 * @brief Initialize V4-front compiler context
 *
 * Creates a compiler context for stateful compilation.
 *
 * @return 0 on success, -1 on failure
 */
static int init_compiler(void) {
  g_compiler_ctx = v4front_context_create();
  if (!g_compiler_ctx) {
    printf("ERROR: Failed to create compiler context\n");
    return -1;
  }

  printf("Compiler initialized\n");
  return 0;
}

/**
 * @brief Initialize REPL context
 *
 * Creates a REPL context that manages the read-eval-print loop.
 *
 * @return 0 on success, -1 on failure
 */
static int init_repl(void) {
  V4ReplConfig config = {
      .vm = g_vm,
      .front_ctx = g_compiler_ctx,
      .line_buffer_size = LINE_BUFFER_SIZE,
  };

  g_repl = v4_repl_create(&config);
  if (!g_repl) {
    printf("ERROR: Failed to create REPL context\n");
    return -1;
  }

  printf("REPL initialized\n");
  return 0;
}

/**
 * @brief Echo character back to UART
 *
 * Provides visual feedback for user input.
 *
 * @param c Character to echo
 */
static void echo_char(char c) {
  v4_hal_uart_putc(UART_PORT, c);
}

/**
 * @brief Print prompt
 */
static void print_prompt(void) {
  printf("v4> ");
}

/**
 * @brief Process complete line of input
 *
 * Compiles and executes the line, displaying results.
 */
static void process_line(void) {
  /* Null-terminate the line */
  g_line_buffer[g_line_pos] = '\0';

  /* Process the line */
  v4_err err = v4_repl_process_line(g_repl, g_line_buffer);

  if (err != 0) {
    /* Display error */
    const char *error = v4_repl_get_error(g_repl);
    if (error) {
      printf("\n%s\n", error);
    } else {
      printf("\nError: %d\n", err);
    }
  } else {
    /* Display stack */
    v4_repl_print_stack(g_repl);
  }

  /* Reset line buffer */
  g_line_pos = 0;

  /* Print prompt for next line */
  print_prompt();
}

/**
 * @brief Handle special control characters
 *
 * @param c Character to handle
 * @return 1 if handled, 0 if not
 */
static int handle_control_char(char c) {
  switch (c) {
    case '\r':
    case '\n':
      /* Process complete line */
      echo_char('\n');
      process_line();
      return 1;

    case 0x08:  /* Backspace */
    case 0x7F:  /* DEL */
      if (g_line_pos > 0) {
        g_line_pos--;
        /* Echo backspace sequence */
        v4_hal_uart_putc(UART_PORT, '\b');
        v4_hal_uart_putc(UART_PORT, ' ');
        v4_hal_uart_putc(UART_PORT, '\b');
      }
      return 1;

    case 0x03:  /* Ctrl+C */
      /* Clear line and reset */
      printf("\n^C\n");
      g_line_pos = 0;
      vm_ds_clear(g_vm);
      print_prompt();
      return 1;

    default:
      return 0;
  }
}

/**
 * @brief Main REPL loop
 *
 * Reads characters from UART, accumulates lines, and processes them.
 */
static void repl_loop(void) {
  printf("\nV4-REPL for ESP32-C6\n");
  printf("Type Forth commands and press Enter\n");
  printf("Ctrl+C to clear stack\n\n");

  print_prompt();

  while (1) {
    char c;

    /* Try to read a character from UART */
    if (v4_hal_uart_getc(UART_PORT, &c) == 0) {
      /* Character received */

      /* Handle control characters */
      if (handle_control_char(c)) {
        continue;
      }

      /* Check for printable ASCII */
      if (c >= 0x20 && c < 0x7F) {
        /* Add to buffer if space available */
        if (g_line_pos < LINE_BUFFER_SIZE - 1) {
          g_line_buffer[g_line_pos++] = c;
          echo_char(c);
        } else {
          /* Buffer full */
          printf("\n[Line too long]\n");
          g_line_pos = 0;
          print_prompt();
        }
      }
    }

    /* Small delay to avoid busy-waiting */
    v4_hal_delay_ms(10);
  }
}

/**
 * @brief Application entry point
 */
void app_main(void) {
  printf("\n=== V4-REPL ESP32-C6 Example ===\n\n");

  /* Initialize VM */
  if (init_vm() != 0) {
    printf("FATAL: VM initialization failed\n");
    return;
  }

  /* Initialize compiler */
  if (init_compiler() != 0) {
    printf("FATAL: Compiler initialization failed\n");
    vm_destroy(g_vm);
    return;
  }

  /* Initialize REPL */
  if (init_repl() != 0) {
    printf("FATAL: REPL initialization failed\n");
    v4front_context_destroy(g_compiler_ctx);
    vm_destroy(g_vm);
    return;
  }

  /* Run REPL loop */
  repl_loop();

  /* Cleanup (never reached in this example) */
  v4_repl_destroy(g_repl);
  v4front_context_destroy(g_compiler_ctx);
  vm_destroy(g_vm);
}
