/**
 * @file test_libv4repl.cpp
 * @brief Unit tests for libv4repl (platform-independent REPL library)
 *
 * Tests the C API provided by libv4repl for integrating V4 REPL into
 * embedded and desktop platforms.
 */

#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

extern "C" {
#include "v4repl/repl.h"
#include "v4/vm_api.h"
#include "v4front/compile.h"
}

#include <cstring>

/**
 * Test fixture for libv4repl tests
 * Creates VM and compiler context for each test
 */
class V4ReplFixture {
protected:
    static constexpr size_t VM_MEMORY_SIZE = 16 * 1024;  // 16KB
    static constexpr size_t ARENA_SIZE = 4 * 1024;        // 4KB

    uint8_t vm_memory[VM_MEMORY_SIZE];
    uint8_t arena_buffer[ARENA_SIZE];
    V4Arena arena;
    struct Vm* vm;
    V4FrontContext* compiler_ctx;
    V4ReplContext* repl;

    void setup() {
        // Initialize arena
        v4_arena_init(&arena, arena_buffer, ARENA_SIZE);

        // Create VM
        VmConfig vm_config;
        vm_config.mem = vm_memory;
        vm_config.mem_size = VM_MEMORY_SIZE;
        vm_config.mmio = nullptr;
        vm_config.mmio_count = 0;
        vm_config.arena = &arena;
        vm = vm_create(&vm_config);
        REQUIRE(vm != nullptr);

        // Create compiler context
        compiler_ctx = v4front_context_create();
        REQUIRE(compiler_ctx != nullptr);

        // Create REPL context
        V4ReplConfig repl_config;
        repl_config.vm = vm;
        repl_config.front_ctx = compiler_ctx;
        repl_config.line_buffer_size = 512;
        repl = v4_repl_create(&repl_config);
        REQUIRE(repl != nullptr);
    }

    void teardown() {
        if (repl) {
            v4_repl_destroy(repl);
            repl = nullptr;
        }
        if (compiler_ctx) {
            v4front_context_destroy(compiler_ctx);
            compiler_ctx = nullptr;
        }
        if (vm) {
            vm_destroy(vm);
            vm = nullptr;
        }
    }

    ~V4ReplFixture() {
        teardown();
    }
};

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Create and destroy REPL context") {
    setup();

    SUBCASE("REPL context is created successfully") {
        CHECK(repl != nullptr);
    }

    SUBCASE("Version is valid") {
        int version = v4_repl_version();
        CHECK(version >= 0x000100);  // At least v0.1.0
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Process simple arithmetic") {
    setup();

    SUBCASE("Add two numbers") {
        v4_err err = v4_repl_process_line(repl, "2 3 +");
        CHECK(err == 0);
        CHECK(v4_repl_stack_depth(repl) == 1);

        // Verify result
        v4_i32 result;
        err = vm_ds_pop(vm, &result);
        CHECK(err == 0);
        CHECK(result == 5);
    }

    SUBCASE("Multiply and subtract") {
        v4_err err = v4_repl_process_line(repl, "10 5 - 2 *");
        CHECK(err == 0);
        CHECK(v4_repl_stack_depth(repl) == 1);

        v4_i32 result;
        err = vm_ds_pop(vm, &result);
        CHECK(err == 0);
        CHECK(result == 10);  // (10-5)*2 = 10
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Word definitions persist across lines") {
    setup();

    SUBCASE("Define and use SQUARE") {
        // Define SQUARE
        v4_err err = v4_repl_process_line(repl, ": SQUARE DUP * ;");
        CHECK(err == 0);

        // Use SQUARE
        err = v4_repl_process_line(repl, "5 SQUARE");
        CHECK(err == 0);
        CHECK(v4_repl_stack_depth(repl) == 1);

        v4_i32 result;
        err = vm_ds_pop(vm, &result);
        CHECK(err == 0);
        CHECK(result == 25);
    }

    SUBCASE("Chain multiple word definitions") {
        v4_err err = v4_repl_process_line(repl, ": DOUBLE 2 * ;");
        CHECK(err == 0);

        err = v4_repl_process_line(repl, ": QUADRUPLE DOUBLE DOUBLE ;");
        CHECK(err == 0);

        err = v4_repl_process_line(repl, "3 QUADRUPLE");
        CHECK(err == 0);

        v4_i32 result;
        err = vm_ds_pop(vm, &result);
        CHECK(err == 0);
        CHECK(result == 12);
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Stack preservation") {
    setup();

    SUBCASE("Stack preserved across multiple operations") {
        v4_err err = v4_repl_process_line(repl, "10 20");
        CHECK(err == 0);
        CHECK(v4_repl_stack_depth(repl) == 2);

        err = v4_repl_process_line(repl, ": DOUBLE 2 * ;");
        CHECK(err == 0);
        CHECK(v4_repl_stack_depth(repl) == 2);  // Stack preserved

        err = v4_repl_process_line(repl, "30 DOUBLE");
        CHECK(err == 0);
        CHECK(v4_repl_stack_depth(repl) == 3);

        // Verify stack contents
        v4_i32 val3, val2, val1;
        vm_ds_pop(vm, &val3);
        vm_ds_pop(vm, &val2);
        vm_ds_pop(vm, &val1);
        CHECK(val1 == 10);
        CHECK(val2 == 20);
        CHECK(val3 == 60);
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: RECURSE support") {
    setup();

    SUBCASE("Factorial with RECURSE") {
        v4_err err = v4_repl_process_line(repl, ": FACTORIAL DUP 1 > IF DUP 1 - RECURSE * THEN ;");
        CHECK(err == 0);

        err = v4_repl_process_line(repl, "5 FACTORIAL");
        CHECK(err == 0);

        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == 120);
    }

    SUBCASE("Fibonacci with RECURSE") {
        v4_err err = v4_repl_process_line(repl,
            ": FIB DUP 2 < IF DROP 1 ELSE DUP 1 - RECURSE SWAP 2 - RECURSE + THEN ;");
        CHECK(err == 0);

        err = v4_repl_process_line(repl, "7 FIB");
        CHECK(err == 0);

        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == 21);
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: New features (V4-front v0.3.x)") {
    setup();

    SUBCASE("Extended arithmetic: 1+ and 1-") {
        v4_err err = v4_repl_process_line(repl, "5 1+");
        CHECK(err == 0);
        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == 6);

        err = v4_repl_process_line(repl, "5 1-");
        CHECK(err == 0);
        vm_ds_pop(vm, &result);
        CHECK(result == 4);
    }

    SUBCASE("Bitwise operations: LSHIFT, RSHIFT") {
        v4_err err = v4_repl_process_line(repl, "1 3 LSHIFT");
        CHECK(err == 0);
        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == 8);

        err = v4_repl_process_line(repl, "8 2 RSHIFT");
        CHECK(err == 0);
        vm_ds_pop(vm, &result);
        CHECK(result == 2);
    }

    SUBCASE("Stack manipulation: ROT, NIP, TUCK") {
        v4_err err = v4_repl_process_line(repl, "1 2 3 ROT");
        CHECK(err == 0);
        v4_i32 val3, val2, val1;
        vm_ds_pop(vm, &val3);
        vm_ds_pop(vm, &val2);
        vm_ds_pop(vm, &val1);
        CHECK(val1 == 2);
        CHECK(val2 == 3);
        CHECK(val3 == 1);

        err = v4_repl_process_line(repl, "1 2 NIP");
        CHECK(err == 0);
        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == 2);
    }

    SUBCASE("Comparison: 0=, 0<, 0>") {
        v4_err err = v4_repl_process_line(repl, "0 0=");
        CHECK(err == 0);
        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == -1);  // TRUE

        err = v4_repl_process_line(repl, "-5 0<");
        CHECK(err == 0);
        vm_ds_pop(vm, &result);
        CHECK(result == -1);  // TRUE

        err = v4_repl_process_line(repl, "5 0>");
        CHECK(err == 0);
        vm_ds_pop(vm, &result);
        CHECK(result == -1);  // TRUE
    }

    SUBCASE("Utilities: ABS, MIN, MAX") {
        v4_err err = v4_repl_process_line(repl, "-5 ABS");
        CHECK(err == 0);
        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == 5);

        err = v4_repl_process_line(repl, "3 7 MIN");
        CHECK(err == 0);
        vm_ds_pop(vm, &result);
        CHECK(result == 3);

        err = v4_repl_process_line(repl, "3 7 MAX");
        CHECK(err == 0);
        vm_ds_pop(vm, &result);
        CHECK(result == 7);
    }

    SUBCASE("Boolean constants: TRUE, FALSE") {
        v4_err err = v4_repl_process_line(repl, "TRUE");
        CHECK(err == 0);
        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == -1);

        err = v4_repl_process_line(repl, "FALSE");
        CHECK(err == 0);
        vm_ds_pop(vm, &result);
        CHECK(result == 0);
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Error handling") {
    setup();

    SUBCASE("Unknown word returns error") {
        v4_err err = v4_repl_process_line(repl, "UNKNOWN_WORD");
        CHECK(err != 0);
    }

    SUBCASE("Invalid syntax returns error") {
        v4_err err = v4_repl_process_line(repl, ": INCOMPLETE");
        CHECK(err != 0);
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Reset operations") {
    setup();

    SUBCASE("Reset clears stack and dictionary") {
        // Add data to stack
        v4_err err = v4_repl_process_line(repl, "10 20 30");
        CHECK(v4_repl_stack_depth(repl) == 3);

        // Define a word
        err = v4_repl_process_line(repl, ": DOUBLE 2 * ;");
        CHECK(err == 0);

        // Reset everything
        v4_repl_reset(repl);
        CHECK(v4_repl_stack_depth(repl) == 0);

        // DOUBLE should no longer be defined
        err = v4_repl_process_line(repl, "5 DOUBLE");
        CHECK(err != 0);  // Error: DOUBLE not found
    }

    SUBCASE("Reset dictionary only keeps stack") {
        // Add data to stack
        v4_err err = v4_repl_process_line(repl, "10 20 30");
        CHECK(v4_repl_stack_depth(repl) == 3);

        // Define a word
        err = v4_repl_process_line(repl, ": DOUBLE 2 * ;");
        CHECK(err == 0);

        // Reset dictionary only
        v4_repl_reset_dictionary(repl);
        CHECK(v4_repl_stack_depth(repl) == 3);  // Stack preserved

        // DOUBLE should no longer be defined
        err = v4_repl_process_line(repl, "5 DOUBLE");
        CHECK(err != 0);  // Error: DOUBLE not found
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Empty line handling") {
    setup();

    v4_err err = v4_repl_process_line(repl, "");
    CHECK(err == 0);
    CHECK(v4_repl_stack_depth(repl) == 0);
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Multiple consecutive operations") {
    setup();

    const char* lines[] = {
        "1 2 +",
        "3 *",
        "4 +",
        "2 /",
    };

    for (const char* line : lines) {
        v4_err err = v4_repl_process_line(repl, line);
        CHECK(err == 0);
    }

    CHECK(v4_repl_stack_depth(repl) == 1);
    v4_i32 result;
    vm_ds_pop(vm, &result);
    CHECK(result == 6);  // ((1+2)*3+4)/2 = 13/2 = 6 (integer division)
}

TEST_CASE("libv4repl: NULL parameter handling") {
    SUBCASE("Create with NULL config fails") {
        V4ReplContext* repl = v4_repl_create(nullptr);
        CHECK(repl == nullptr);
    }

    SUBCASE("Destroy NULL context is safe") {
        v4_repl_destroy(nullptr);
        // Should not crash
        CHECK(true);
    }
}

TEST_CASE_FIXTURE(V4ReplFixture, "libv4repl: Task system support (V4 v0.9.1, V4-front v0.5.0)") {
    setup();

    SUBCASE("Task control words compile successfully") {
        // SPAWN: Create new task
        v4_err err = v4_repl_process_line(repl, ": TASK-TEST 100 MS ;");
        CHECK(err == 0);

        // SLEEP/MS: Task sleep
        err = v4_repl_process_line(repl, ": SLEEPER 50 MS ;");
        CHECK(err == 0);
        err = v4_repl_process_line(repl, ": SLEEPER2 100 SLEEP ;");
        CHECK(err == 0);

        // YIELD/PAUSE: Yield CPU
        err = v4_repl_process_line(repl, ": YIELDER YIELD ;");
        CHECK(err == 0);
        err = v4_repl_process_line(repl, ": PAUSER PAUSE ;");
        CHECK(err == 0);

        // TASK-EXIT: Exit task
        err = v4_repl_process_line(repl, ": EXITER TASK-EXIT ;");
        CHECK(err == 0);
    }

    SUBCASE("Critical section words compile successfully") {
        v4_err err = v4_repl_process_line(repl, ": PROTECTED CRITICAL 42 UNCRITICAL ;");
        CHECK(err == 0);
    }

    SUBCASE("Message passing words compile successfully") {
        // SEND: Send message to task
        v4_err err = v4_repl_process_line(repl, ": SENDER 1 2 3 SEND DROP ;");
        CHECK(err == 0);

        // RECEIVE: Non-blocking receive
        err = v4_repl_process_line(repl, ": RECEIVER 1 RECEIVE DROP DROP DROP ;");
        CHECK(err == 0);

        // RECEIVE-BLOCKING: Blocking receive with timeout
        err = v4_repl_process_line(repl, ": BLOCKER 1 100 RECEIVE-BLOCKING DROP DROP DROP ;");
        CHECK(err == 0);
    }

    SUBCASE("Task introspection words compile successfully") {
        // ME: Get current task ID
        v4_err err = v4_repl_process_line(repl, "ME");
        CHECK(err == 0);
        v4_i32 task_id;
        vm_ds_pop(vm, &task_id);
        CHECK(task_id >= 0);  // Valid task ID (0 = main task)

        // TASKS: Get task count
        err = v4_repl_process_line(repl, "TASKS");
        CHECK(err == 0);
        v4_i32 task_count;
        vm_ds_pop(vm, &task_count);
        CHECK(task_count >= 0);  // Task count is valid (0 or more)
    }

    SUBCASE("Basic task functionality") {
        // Test ME returns consistent task ID
        v4_err err = v4_repl_process_line(repl, "ME ME =");
        CHECK(err == 0);
        v4_i32 result;
        vm_ds_pop(vm, &result);
        CHECK(result == -1);  // TRUE (-1)
    }
}
