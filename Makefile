CC := gcc

CFLAGS  := -Wall -Wextra -Wpedantic -g
INCLUDES := -Iinclude

LIBFLAGS := -shared -fPIC
LIBS := -ldl -pthread

BUILD_DIR := build
TEST_DIR := tests

LIB := $(BUILD_DIR)/libprofiler.so

TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_NAMES := $(notdir $(basename $(TEST_SRCS)))
TEST_BINS := $(addprefix $(BUILD_DIR)/,$(TEST_NAMES))

.PHONY: all clean tests run list

all: $(LIB) tests

# ============================================================
# Lib
# ============================================================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB): profiler.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBFLAGS) \
		-o $@ $< $(LIBS)

# ============================================================
# Tests
# ============================================================

tests: $(TEST_BINS)

$(BUILD_DIR)/%: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) \
		-o $@ $< -pthread

# ============================================================
# Running rests
# ============================================================

run: all
	@for t in $(TEST_BINS); do \
		echo ""; \
		echo "================================================"; \
		echo "Running $$t"; \
		echo "================================================"; \
		LD_PRELOAD=$(abspath $(LIB)) $$t; \
	done

# ============================================================
# Dynamiczne targety:
# make run-malloc_test
# make run-thread_test
# ============================================================

run-%: $(BUILD_DIR)/% $(LIB)
	@echo "Running $*"
	LD_PRELOAD=$(abspath $(LIB)) $(BUILD_DIR)/$*

# ============================================================
# Test list
# ============================================================

list:
	@printf '%s\n' $(TEST_NAMES)

clean:
	rm -rf $(BUILD_DIR)