#include "meta_commands.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <v4/internal/vm.h>  // For Word structure definition

MetaCommands::MetaCommands(struct Vm* vm, V4FrontContext* ctx) : vm_(vm), ctx_(ctx) {}

bool MetaCommands::execute(const char* line) {
  // Skip leading whitespace
  while (*line == ' ' || *line == '\t') {
    line++;
  }

  // Check if it's a meta-command
  if (*line != '.') {
    return false;  // Not a meta-command
  }

  line++;  // Skip the '.'

  // Match command
  if (strncmp(line, "words", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
    cmd_words();
  } else if (strncmp(line, "stack", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
    cmd_stack();
  } else if (strncmp(line, "rstack", 6) == 0 && (line[6] == '\0' || line[6] == ' ')) {
    cmd_rstack();
  } else if (strncmp(line, "dump", 4) == 0 && (line[4] == '\0' || line[4] == ' ')) {
    cmd_dump(line + 4);  // Pass arguments after "dump"
  } else if (strncmp(line, "see", 3) == 0 && (line[3] == '\0' || line[3] == ' ')) {
    cmd_see(line + 3);  // Pass arguments after "see"
  } else if (strncmp(line, "reset", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
    cmd_reset();
  } else if (strncmp(line, "memory", 6) == 0 && (line[6] == '\0' || line[6] == ' ')) {
    cmd_memory();
  } else if (strncmp(line, "help", 4) == 0 && (line[4] == '\0' || line[4] == ' ')) {
    cmd_help();
  } else if (strncmp(line, "version", 7) == 0 && (line[7] == '\0' || line[7] == ' ')) {
    cmd_version();
  } else {
    // Unknown meta-command
    printf("Unknown meta-command: .%s\n", line);
    printf("Type .help for available commands\n");
  }

  return true;
}

void MetaCommands::cmd_words() {
  int count = v4front_context_get_word_count(ctx_);

  if (count == 0) {
    printf("No words defined.\n");
    return;
  }

  printf("Defined words (%d):\n", count);
  for (int i = 0; i < count; i++) {
    const char* name = v4front_context_get_word_name(ctx_, i);
    if (name) {
      printf("  %s\n", name);
    }
  }
}

void MetaCommands::cmd_stack() {
  int ds_depth = vm_ds_depth_public(vm_);

  printf("Data Stack (depth: %d):\n", ds_depth);
  if (ds_depth == 0) {
    printf("  <empty>\n");
  } else {
    // Print from bottom to top (index 0 = bottom)
    for (int i = ds_depth - 1; i >= 0; i--) {
      v4_i32 val = vm_ds_peek_public(vm_, i);
      printf("  [%d]: %d (0x%08X)\n", ds_depth - 1 - i, val, (unsigned int) val);
    }
  }

  // Return stack display
  int rs_depth = vm_rs_depth_public(vm_);
  printf("\nReturn Stack (depth: %d):\n", rs_depth);
  if (rs_depth == 0) {
    printf("  <empty>\n");
  } else {
    // Get return stack contents
    v4_i32 rs_data[64];  // Max return stack size
    int count = vm_rs_copy_to_array(vm_, rs_data, 64);

    // Print from bottom to top
    for (int i = count - 1; i >= 0; i--) {
      printf("  [%d]: 0x%08X\n", count - 1 - i, (unsigned int) rs_data[i]);
    }
  }
}

void MetaCommands::cmd_rstack() {
  int rs_depth = vm_rs_depth_public(vm_);

  printf("Return Stack (depth: %d / 64):\n", rs_depth);
  if (rs_depth == 0) {
    printf("  <empty>\n");
    return;
  }

  // Get return stack contents
  v4_i32 rs_data[64];  // Max return stack size
  int count = vm_rs_copy_to_array(vm_, rs_data, 64);

  printf("\nCall trace (most recent first):\n");
  for (int i = count - 1; i >= 0; i--) {
    printf("  [%2d]: 0x%08X", count - 1 - i, (unsigned int) rs_data[i]);

    // Try to resolve address to word name (if it's a return address)
    // TODO: Add word lookup by code address when V4-front API is available

    printf("\n");
  }

  printf("\nNote: Values shown are return addresses from function calls.\n");
  printf("      Use .stack to see both data and return stacks together.\n");
}

void MetaCommands::cmd_dump(const char* args) {
  v4_u32 addr = last_dump_addr_;
  v4_u32 length = 256;  // Default: 256 bytes

  // Parse arguments: .dump [addr] [length]
  while (*args == ' ') args++;  // Skip leading spaces

  if (*args != '\0') {
    // Parse address
    char* end;
    addr = (v4_u32)strtoul(args, &end, 0);  // Auto-detect hex (0x prefix) or decimal
    args = end;

    while (*args == ' ') args++;  // Skip spaces

    if (*args != '\0') {
      // Parse length
      length = (v4_u32)strtoul(args, &end, 0);
    }
  }

  // Align address to 4-byte boundary for cleaner output
  v4_u32 aligned_addr = addr & ~3;

  printf("Memory dump at 0x%08X (%u bytes):\n", aligned_addr, length);
  printf("Address   +0 +1 +2 +3  +4 +5 +6 +7  +8 +9 +A +B  +C +D +E +F  ASCII\n");
  printf("--------  -----------  -----------  -----------  -----------  ----------------\n");

  for (v4_u32 offset = 0; offset < length; offset += 16) {
    printf("%08X  ", aligned_addr + offset);

    // Read and display 16 bytes in hex
    v4_u8 bytes[16];
    for (int i = 0; i < 16; i++) {
      v4_u32 byte_addr = aligned_addr + offset + i;
      v4_u32 word;

      // Read 32-bit word (VM memory API works in 32-bit units)
      v4_err err = vm_mem_read32(vm_, byte_addr & ~3, &word);

      if (err == 0) {
        // Extract the byte from the word (little-endian)
        int byte_pos = byte_addr & 3;
        bytes[i] = (word >> (byte_pos * 8)) & 0xFF;
        printf("%02X ", bytes[i]);
      } else {
        bytes[i] = 0;
        printf("?? ");
      }

      // Add spacing every 4 bytes
      if ((i & 3) == 3) printf(" ");
    }

    // Display ASCII representation
    printf(" ");
    for (int i = 0; i < 16; i++) {
      char c = bytes[i];
      printf("%c", (c >= 32 && c < 127) ? c : '.');
    }
    printf("\n");

    // Stop if we've gone beyond requested length
    if (offset + 16 >= length) break;
  }

  // Update last dump address for next invocation
  last_dump_addr_ = aligned_addr + ((length + 15) & ~15);  // Round up to next 16-byte boundary

  printf("\nNext: .dump (continues from 0x%08X)\n", last_dump_addr_);
}

void MetaCommands::cmd_see(const char* args) {
  // Parse word name from arguments
  while (*args == ' ') args++;  // Skip leading spaces

  if (*args == '\0') {
    printf("Usage: .see <word_name>\n");
    printf("Example: .see SQUARE\n");
    return;
  }

  // Extract word name (up to first space or end of string)
  char word_name[64];
  int i = 0;
  while (*args && *args != ' ' && i < 63) {
    word_name[i++] = *args++;
  }
  word_name[i] = '\0';

  // Find word in compiler context
  int vm_idx = v4front_context_find_word(ctx_, word_name);
  if (vm_idx < 0) {
    printf("Word '%s' not found.\n", word_name);
    printf("Use .words to see all defined words.\n");
    return;
  }

  // Get word from VM
  Word* word = vm_get_word(vm_, vm_idx);
  if (!word || !word->code || word->code_len == 0) {
    printf("Word '%s' has no bytecode.\n", word_name);
    return;
  }

  // Display word header
  printf("Word: %s\n", word_name);
  printf("VM index: %d\n", vm_idx);
  printf("Bytecode length: %d bytes\n", word->code_len);
  printf("\nDisassembly:\n");
  printf("Offset  Bytes                    \n");
  printf("------  -------------------------\n");

  // Display bytecode in hex (16 bytes per line)
  const uint8_t* code = word->code;
  for (uint32_t offset = 0; offset < word->code_len; offset += 16) {
    printf("%04X    ", offset);

    // Print hex bytes
    for (uint32_t i = 0; i < 16 && (offset + i) < word->code_len; i++) {
      printf("%02X ", code[offset + i]);
    }

    printf("\n");
  }

  printf("\nNote: Use V4-front disassembler for opcode names.\n");
  printf("      Bytecode is in V4 instruction format.\n");
}

void MetaCommands::cmd_reset() {
  vm_reset(vm_);
  v4front_context_reset(ctx_);
  printf("VM and compiler context reset.\n");
  last_dump_addr_ = 0;  // Reset dump address too
}

void MetaCommands::cmd_memory() {
  printf("Memory usage information:\n");
  printf("  VM memory size: (not yet available from V4-core)\n");
  printf("  Data stack depth: %d / 256\n", vm_ds_depth_public(vm_));
  printf("  Return stack depth: %d / 64\n", vm_rs_depth_public(vm_));
  printf("  Registered words: %d\n", v4front_context_get_word_count(ctx_));
}

void MetaCommands::cmd_help() {
  printf("V4 REPL Help\n");
  printf("════════════════════════════════════════════════════════════════\n\n");

  printf("Meta-commands:\n");
  printf("  .words              - List all defined words\n");
  printf("  .stack              - Show data and return stack contents\n");
  printf("  .rstack             - Show return stack with call trace\n");
  printf("  .dump [addr] [len]  - Hexdump memory (default: continue from last)\n");
  printf("  .see <word>         - Show word bytecode disassembly\n");
  printf("  .reset              - Reset VM and compiler context\n");
  printf("  .memory             - Show memory usage statistics\n");
  printf("  .help               - Show this help message\n");
  printf("  .version            - Show REPL and component versions\n");

  printf("\nPASTE mode (multi-line input):\n");
  printf("  <<<        - Enter PASTE mode for multi-line definitions\n");
  printf("  >>>        - Exit PASTE mode and compile buffered input\n");

  printf("\nControl:\n");
  printf("  Ctrl+D     - Exit REPL\n");
  printf("  Ctrl+C     - Interrupt execution (if supported)\n");
  printf("  bye / quit - Exit REPL\n");

  printf("\nBasic Forth:\n");
  printf("  Numbers    - Push values to stack (e.g., '42')\n");
  printf("  + - * /    - Arithmetic operations\n");
  printf("  DUP        - Duplicate top of stack\n");
  printf("  DROP       - Remove top of stack\n");
  printf("  SWAP       - Swap top two values\n");
  printf("  .          - Print and pop top of stack\n");

  printf("\nWord definitions:\n");
  printf("  : NAME ... ; - Define a new word\n");
  printf("  Example: : SQUARE DUP * ;\n");

  printf("\n════════════════════════════════════════════════════════════════\n");
}

void MetaCommands::cmd_version() {
  printf("V4 REPL v0.2.0\n");
  printf("════════════════════════════════════════════════════════════════\n");
  printf("Components:\n");
  printf("  V4-core:   (version API not yet available)\n");
  printf("  V4-front:  (version API not yet available)\n");
  printf("  linenoise: integrated\n");
  printf("\nBuild configuration:\n");
#ifdef WITH_FILESYSTEM
  printf("  Filesystem: enabled (history support)\n");
#else
  printf("  Filesystem: disabled (no history support)\n");
#endif
  printf("  C++ standard: C++17\n");
  printf("  Exceptions: disabled (-fno-exceptions)\n");
  printf("  RTTI: disabled (-fno-rtti)\n");
  printf("════════════════════════════════════════════════════════════════\n");
}
