#pragma once

#include <v4/vm_api.h>
#include <v4front/compile.h>

/**
 * @brief Meta-command handler for V4 REPL
 *
 * Provides dot-commands for inspecting and controlling the REPL state:
 * - .words              : List all defined words
 * - .stack              : Show data and return stack contents
 * - .rstack             : Show return stack with call trace
 * - .dump [addr] [len]  : Hexdump memory (default: continue from last)
 * - .see <word>         : Show word bytecode disassembly
 * - .reset              : Reset VM and compiler context
 * - .memory             : Show memory usage statistics
 * - .help               : Show help message
 * - .version            : Show version information
 */
class MetaCommands {
 public:
  /**
   * @brief Construct a new MetaCommands handler
   *
   * @param vm Pointer to VM instance
   * @param ctx Pointer to compiler context
   */
  MetaCommands(struct Vm* vm, V4FrontContext* ctx);

  /**
   * @brief Execute a meta-command if the line starts with '.'
   *
   * @param line Input line to check and execute
   * @return true if line was a meta-command (executed), false otherwise
   */
  bool execute(const char* line);

 private:
  struct Vm* vm_;
  V4FrontContext* ctx_;
  v4_u32 last_dump_addr_ = 0;  // Track last dump address for continuation

  void cmd_words();
  void cmd_stack();
  void cmd_rstack();
  void cmd_dump(const char* args);
  void cmd_see(const char* args);
  void cmd_reset();
  void cmd_memory();
  void cmd_help();
  void cmd_version();
};
