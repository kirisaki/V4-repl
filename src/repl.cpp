#include "repl.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "linenoise.h"
}

#ifdef WITH_FILESYSTEM
#include <unistd.h>
#include <sys/stat.h>
#endif

static const char* PROMPT = "v4> ";
static const int MAX_HISTORY = 1000;

Repl::Repl() : vm_(nullptr) {
  // Initialize VM memory to zero
  memset(vm_memory_, 0, sizeof(vm_memory_));

  // Create VM configuration
  VmConfig cfg;
  cfg.mem = vm_memory_;
  cfg.mem_size = sizeof(vm_memory_);
  cfg.mmio = nullptr;
  cfg.mmio_count = 0;

  // Create VM instance
  vm_ = vm_create(&cfg);
  if (!vm_) {
    fprintf(stderr, "Failed to create VM\n");
    exit(1);
  }

#ifdef WITH_FILESYSTEM
  init_history();
#endif
}

Repl::~Repl() {
#ifdef WITH_FILESYSTEM
  save_history();
#endif

  if (vm_) {
    vm_destroy(vm_);
    vm_ = nullptr;
  }
}

#ifdef WITH_FILESYSTEM
void Repl::init_history() {
  // Get home directory
  const char* home = getenv("HOME");
  if (!home) {
    home = ".";
  }

  // Construct history file path
  snprintf(history_path_, sizeof(history_path_), "%s/.v4_history", home);

  // Set max history
  linenoiseHistorySetMaxLen(MAX_HISTORY);

  // Load existing history
  linenoiseHistoryLoad(history_path_);
}

void Repl::save_history() {
  linenoiseHistorySave(history_path_);
}
#endif

void Repl::print_stack() {
  int depth = vm_ds_depth_public(vm_);

  if (depth == 0) {
    printf(" ok\n");
    return;
  }

  printf(" ok [%d]:", depth);

  // Print stack from bottom to top
  for (int i = depth - 1; i >= 0; --i) {
    v4_i32 val = vm_ds_peek_public(vm_, i);
    printf(" %d", val);
  }

  printf("\n");
}

void Repl::print_error(const char* msg, int code) {
  if (code != 0) {
    fprintf(stderr, "Error [%d]: %s\n", code, msg);
  } else {
    fprintf(stderr, "Error: %s\n", msg);
  }
}

int Repl::eval_line(const char* line) {
  // Check for exit command
  if (strcmp(line, "bye") == 0 || strcmp(line, "quit") == 0) {
    return 1;  // Signal to exit
  }

  // Skip empty lines
  if (line[0] == '\0' || line[0] == '\n') {
    return 0;
  }

  // Compile the input
  V4FrontBuf buf;
  memset(&buf, 0, sizeof(buf));

  char errmsg[256];
  v4front_err err = v4front_compile(line, &buf, errmsg, sizeof(errmsg));

  if (err != 0) {
    print_error(errmsg, err);
    return -1;
  }

  // If word definitions are present, reset VM
  // This ensures word indices start from 0, matching V4-front's compilation
  // Note: This clears the stack as a Phase 1 limitation
  if (buf.word_count > 0) {
    vm_reset(vm_);
  }

  // Register any defined words FIRST (before executing main code)
  for (int i = 0; i < buf.word_count; ++i) {
    V4FrontWord* word = &buf.words[i];
    int wid = vm_register_word(vm_, word->name, word->code,
                                static_cast<int>(word->code_len));

    if (wid < 0) {
      print_error("Failed to register word definition", wid);
      v4front_free(&buf);
      return -1;
    }
  }

  // Register and execute main code
  if (buf.data && buf.size > 0) {
    int wid = vm_register_word(vm_, nullptr, buf.data, static_cast<int>(buf.size));

    if (wid < 0) {
      print_error("Failed to register word", wid);
      v4front_free(&buf);
      return -1;
    }

    struct Word* entry = vm_get_word(vm_, wid);
    if (!entry) {
      print_error("Failed to get word entry", 0);
      v4front_free(&buf);
      return -1;
    }

    v4_err exec_err = vm_exec(vm_, entry);
    if (exec_err != 0) {
      print_error("Execution failed", exec_err);
      v4front_free(&buf);
      return -1;
    }
  }

  // Free compiler output
  v4front_free(&buf);

  return 0;  // Success
}

int Repl::run() {
  printf("V4 REPL v0.1.0\n");
  printf("Type 'bye' or press Ctrl+D to exit\n\n");

  while (true) {
    char* line = linenoise(PROMPT);

    // Ctrl+D pressed
    if (!line) {
      printf("\nGoodbye!\n");
      break;
    }

    // Evaluate the line
    int result = eval_line(line);

    if (result == 1) {
      // User requested exit
      printf("Goodbye!\n");
      linenoiseFree(line);
      break;
    } else if (result == 0) {
      // Success - print stack
      print_stack();

#ifdef WITH_FILESYSTEM
      // Add to history if not empty
      if (line[0] != '\0') {
        linenoiseHistoryAdd(line);
      }
#endif
    }
    // result == -1 means error, already printed

    linenoiseFree(line);
  }

  return 0;
}
