NANOCBOR_DIR ?= $(CURDIR)

# cannot simply use CC ?= gcc, as CC defaults to "cc" if undefined on GNU Make
ifeq ($(origin CC),default)
  CC := gcc
endif
RM = rm -rf
TIDY ?= clang-tidy
CFLAGS ?=

INC_GLOBAL ?= /usr/include
INC_DIR = $(NANOCBOR_DIR)/include
SRC_DIR = $(NANOCBOR_DIR)/src

TEST_DIR=tests

BIN_DIR ?= bin
OBJ_DIR ?= $(BIN_DIR)/objs

# Only check at issues present for c99 compatible code
CFLAGS_TIDY ?= -std=c99
TIDYFLAGS=-checks=* -warnings-as-errors=*

CFLAGS_WARN += -Wall -Wextra -pedantic -Werror -Wshadow
CFLAGS += -fPIC $(CFLAGS_WARN) -I$(INC_DIR) -I$(INC_GLOBAL) -Os

SRCS ?= $(wildcard $(SRC_DIR)/*.c)
OBJS ?= $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

lib: $(BIN_DIR)/nanocbor.so

prepare:
	@mkdir -p $(OBJ_DIR)

# Build a binary
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c prepare
	$(CC) $(CFLAGS) -c -o $@ $<

objs: $(OBJS)

$(BIN_DIR)/nanocbor.so: objs
	$(CC) $(CFLAGS) $(OBJS) -o $@ -shared

clang-tidy:
	$(TIDY) $(TIDYFLAGS) $(SRCS) -- $(CFLAGS) $(CFLAGS_TIDY)

clean:
	$(RM) $(BIN_DIR)
