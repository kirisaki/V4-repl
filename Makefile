.PHONY: all build build-fetch build-no-fs release run test clean format format-check help

# Default paths for local V4 and V4-front
V4_PATH ?= ../V4
V4FRONT_PATH ?= ../V4-front

# Default target
all: build

# Build with local V4 and V4-front (default)
build:
	@echo "🔨 Building V4-repl with local dependencies..."
	@cmake -B build -DCMAKE_BUILD_TYPE=Debug \
		-DV4_LOCAL_PATH=$(V4_PATH) \
		-DV4FRONT_LOCAL_PATH=$(V4FRONT_PATH) \
		-DWITH_FILESYSTEM=ON
	@cmake --build build -j

# Build without filesystem support (for embedded)
build-no-fs:
	@echo "🔨 Building V4-repl without filesystem support..."
	@cmake -B build -DCMAKE_BUILD_TYPE=Debug \
		-DV4_LOCAL_PATH=$(V4_PATH) \
		-DV4FRONT_LOCAL_PATH=$(V4FRONT_PATH) \
		-DWITH_FILESYSTEM=OFF
	@cmake --build build -j

# Build by fetching V4 and V4-front from Git
build-fetch:
	@echo "🔨 Building V4-repl (fetching dependencies from Git)..."
	@cmake -B build -DCMAKE_BUILD_TYPE=Debug \
		-DV4_LOCAL_PATH="" \
		-DV4FRONT_LOCAL_PATH="" \
		-DWITH_FILESYSTEM=ON
	@cmake --build build -j

# Release build
release:
	@echo "🚀 Building release..."
	@cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
		-DV4_LOCAL_PATH=$(V4_PATH) \
		-DV4FRONT_LOCAL_PATH=$(V4FRONT_PATH) \
		-DWITH_FILESYSTEM=ON
	@cmake --build build-release -j

# Release build without filesystem
release-no-fs:
	@echo "🚀 Building release without filesystem..."
	@cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
		-DV4_LOCAL_PATH=$(V4_PATH) \
		-DV4FRONT_LOCAL_PATH=$(V4FRONT_PATH) \
		-DWITH_FILESYSTEM=OFF
	@cmake --build build-release -j

# Run the REPL
run: build
	@echo "🚀 Starting V4 REPL..."
	@./build/v4-repl

# Run release build
run-release: release
	@echo "🚀 Starting V4 REPL (release)..."
	@./build-release/v4-repl

# Run basic smoke tests
test: build
	@bash test_smoke.sh

# Clean
clean:
	@echo "🧹 Cleaning..."
	@rm -rf build build-release build-debug build-asan build-ubsan _deps

# Apply formatting
format:
	@echo "✨ Formatting C/C++ code..."
	@find src -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) \
		-not -path "*/vendor/*" -exec clang-format -i {} \;
	@echo "✨ Formatting CMake files..."
	@find . -name 'CMakeLists.txt' -o -name '*.cmake' | xargs cmake-format -i 2>/dev/null || echo "⚠️  cmake-format not found, skipping CMake formatting"
	@echo "✅ Formatting complete!"

# Format check
format-check:
	@echo "🔍 Checking C/C++ formatting..."
	@find src -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' \) \
		-not -path "*/vendor/*" | xargs clang-format --dry-run --Werror || \
		(echo "❌ C/C++ formatting check failed. Run 'make format' to fix." && exit 1)
	@echo "🔍 Checking CMake formatting..."
	@find . -name 'CMakeLists.txt' -o -name '*.cmake' | xargs cmake-format --check 2>/dev/null || \
		echo "⚠️  cmake-format not found, skipping CMake format check"
	@echo "✅ All formatting checks passed!"

# Sanitizer builds
asan:
	@echo "🛡️  Building with AddressSanitizer..."
	@cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
		-DV4_LOCAL_PATH=$(V4_PATH) \
		-DV4FRONT_LOCAL_PATH=$(V4FRONT_PATH) \
		-DWITH_FILESYSTEM=ON \
		-DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"
	@cmake --build build-asan -j
	@echo "🧪 Running smoke tests with AddressSanitizer..."
	@echo -e "1 2 +\nbye" | ./build-asan/v4-repl > /dev/null
	@echo "✅ ASan build passed!"

ubsan:
	@echo "🛡️  Building with UndefinedBehaviorSanitizer..."
	@cmake -B build-ubsan -DCMAKE_BUILD_TYPE=Debug \
		-DV4_LOCAL_PATH=$(V4_PATH) \
		-DV4FRONT_LOCAL_PATH=$(V4FRONT_PATH) \
		-DWITH_FILESYSTEM=ON \
		-DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g"
	@cmake --build build-ubsan -j
	@echo "🧪 Running smoke tests with UndefinedBehaviorSanitizer..."
	@echo -e "1 2 +\nbye" | ./build-ubsan/v4-repl > /dev/null
	@echo "✅ UBSan build passed!"

# Help
help:
	@echo "V4-repl Makefile targets:"
	@echo ""
	@echo "  make / make all      - Build V4-repl (default: uses local V4/V4-front)"
	@echo "  make build           - Build with local dependencies"
	@echo "  make build-no-fs     - Build without filesystem support (embedded)"
	@echo "  make build-fetch     - Build by fetching V4/V4-front from Git"
	@echo "  make release         - Release build"
	@echo "  make release-no-fs   - Release build without filesystem"
	@echo "  make run             - Build and run the REPL interactively"
	@echo "  make run-release     - Run release build"
	@echo "  make test            - Run smoke tests"
	@echo "  make clean           - Remove build directories"
	@echo "  make format          - Format code with clang-format"
	@echo "  make format-check    - Check formatting without modifying files"
	@echo "  make asan            - Build and test with AddressSanitizer"
	@echo "  make ubsan           - Build and test with UndefinedBehaviorSanitizer"
	@echo "  make help            - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  V4_PATH              - Path to V4 source (default: ../V4)"
	@echo "  V4FRONT_PATH         - Path to V4-front source (default: ../V4-front)"
	@echo ""
	@echo "Examples:"
	@echo "  make                                    # Build with local dependencies"
	@echo "  make run                                # Build and run REPL"
	@echo "  make test                               # Run smoke tests"
	@echo "  make build V4_PATH=/path/to/V4          # Use custom V4 path"
	@echo "  make build-no-fs                        # Build for embedded (no filesystem)"
	@echo "  make release                            # Optimized release build"
