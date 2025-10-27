#!/bin/bash
# Smoke tests for V4-repl

set -e

REPL="./build/v4-repl"

echo "🧪 Running smoke tests..."

# Test 1: Basic arithmetic
echo "  Test 1: Basic arithmetic (1 + 2)..."
OUTPUT=$(echo -e "1 2 +\nbye" | $REPL 2>&1)
if echo "$OUTPUT" | grep -qF "ok [1]: 3"; then
    echo "  ✅ Test 1 passed"
else
    echo "  ❌ Test 1 failed"
    echo "$OUTPUT"
    exit 1
fi

# Test 2: Stack operations
echo "  Test 2: Stack operations (10 20 30 + *)..."
OUTPUT=$(echo -e "10 20 30\n+\n*\nbye" | $REPL 2>&1)
if echo "$OUTPUT" | grep -qF "ok [1]: 500"; then
    echo "  ✅ Test 2 passed"
else
    echo "  ❌ Test 2 failed"
    echo "$OUTPUT"
    exit 1
fi

# Test 3: Word definition
echo "  Test 3: Word definition..."
OUTPUT=$(echo -e ": SQUARE DUP * ; 5 SQUARE\nbye" | $REPL 2>&1)
if echo "$OUTPUT" | grep -qF "ok [1]: 25"; then
    echo "  ✅ Test 3 passed"
else
    echo "  ❌ Test 3 failed"
    echo "$OUTPUT"
    exit 1
fi

# Test 4: DUP operation
echo "  Test 4: DUP operation..."
OUTPUT=$(echo -e "5 DUP\n*\nbye" | $REPL 2>&1)
if echo "$OUTPUT" | grep -qF "ok [1]: 25"; then
    echo "  ✅ Test 4 passed"
else
    echo "  ❌ Test 4 failed"
    echo "$OUTPUT"
    exit 1
fi

echo "✅ All smoke tests passed!"
