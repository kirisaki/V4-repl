#pragma once

#include <v4/vm_api.h>
#include <v4front/compile.h>

/**
 * @brief Interactive REPL for V4 Forth VM
 *
 * Provides a read-eval-print loop with optional history support
 * when compiled with WITH_FILESYSTEM=1.
 */
class Repl {
public:
  /**
   * @brief Construct a new REPL instance
   *
   * Initializes VM with default configuration and loads history
   * (if filesystem support is enabled).
   */
  Repl();

  /**
   * @brief Destroy the REPL instance
   *
   * Saves history (if enabled) and frees VM resources.
   */
  ~Repl();

  /**
   * @brief Run the REPL loop
   *
   * Continues until user enters Ctrl+D or 'bye' command.
   *
   * @return Exit code (0 = success)
   */
  int run();

private:
  struct Vm* vm_;
  uint8_t vm_memory_[16384];  // 16KB RAM for VM

#ifdef WITH_FILESYSTEM
  char history_path_[256];
  void init_history();
  void save_history();
#endif

  /**
   * @brief Print current data stack contents
   *
   * Format: " ok [depth]: val1 val2 ... valN"
   */
  void print_stack();

  /**
   * @brief Print error message with context
   *
   * @param msg Error message string
   * @param code Optional error code (default: 0)
   */
  void print_error(const char* msg, int code = 0);

  /**
   * @brief Evaluate a single line of input
   *
   * Compiles and executes the line, handling errors.
   *
   * @param line Input string to evaluate
   * @return 0 on success, -1 on error, 1 to exit
   */
  int eval_line(const char* line);
};
