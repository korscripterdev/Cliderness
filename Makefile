# Cliderness - Open World Survival Sandbox
# Created by korscripterdev
#
# Plain make build for Linux, macOS, FreeBSD, OpenBSD, NetBSD and other Unix
# systems (gcc or clang). CMake is the primary build; this file needs nothing
# but a C compiler and make.
#
#   make            build ./cliderness
#   make test       build and run the test suite
#   make strict     verify the engine compiles as strict ISO C90
#   make clean

CC      ?= cc
CFLAGS  ?= -O2 -g
C90     := -std=c90 -pedantic-errors -Wall -Wextra -Wno-unused-parameter
PLATCF  := -std=c99 -Wall
INC     := -Isrc
BUILD   := build-make

ENGINE_SRCS := $(filter-out src/main.c,$(shell find src -name '*.c' -not -path 'src/platform/*'))
PLAT_SRCS   := src/platform/unix/posix_platform.c src/platform/null/null_platform.c
TEST_SRCS   := $(wildcard tests/*.c)

ENGINE_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(ENGINE_SRCS))
PLAT_OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(PLAT_SRCS))
TEST_OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(TEST_SRCS))
MAIN_OBJ    := $(BUILD)/src/main.o

all: cliderness

cliderness: $(ENGINE_OBJS) $(PLAT_OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clide_tests: $(ENGINE_OBJS) $(PLAT_OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/src/platform/%.o: src/platform/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(PLATCF) $(INC) -c $< -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(C90) $(INC) -c $< -o $@

test: cliderness clide_tests
	cd $(BUILD) && ../clide_tests --data ../data
	./cliderness --version | grep -q korscripterdev
	./cliderness --check-lang --data data > /dev/null
	./cliderness --data data --headless "{WAIT:3}" | grep -q "Created by korscripterdev"
	@echo "all tests passed"

strict:
	@for f in $(ENGINE_SRCS) src/main.c; do $(CC) $(C90) $(INC) -fsyntax-only $$f || exit 1; done
	@echo "engine is strict ISO C90"

clean:
	rm -rf $(BUILD) cliderness clide_tests

.PHONY: all test strict clean
