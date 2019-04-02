NANOCBOR_DIR ?= .

CC ?= gcc
RM ?= rm -rf
TIDY ?= clang-tidy

INC_DIR = $(NANOCBOR_DIR)/include
SRC_DIR = $(NANOCBOR_DIR)/src

TEST_DIR=tests

BIN_DIR ?= bin
OBJ_DIR ?= $(BIN_DIR)/objs

TIDYFLAGS=-checks=* -warnings-as-errors=*

CFLAGS_COVERAGE += -coverage
CFLAGS_DEBUG += $(CFLAGS_COVERAGE) -g3

CFLAGS += -fPIC -Wall -Wextra -pedantic -Werror -I$(INC_DIR) -flto

SRCS += $(wildcard $(SRC_DIR)/*.c)
OBJS ?= $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

lib: $(BIN_DIR)/nanocbor.so

prepare:
	@mkdir -p $(OBJ_DIR)

# Build a binary
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c prepare
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/nanocbor.so: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -shared

clang-tidy:
	$(TIDY) $(TIDYFLAGS) $(TIDYSRCS) -- $(CFLAGS)

clean:
	$(RM) $(BIN_DIR)
