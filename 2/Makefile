# Makefile for seqops (Windows-friendly)
# 所有日志英文，清理命令使用 del 以兼容 Windows。
# All logs in English; use 'del' for Windows cleanup.

CC      := gcc
CFLAGS  := -std=c11 -Og -g
LDFLAGS :=

# Target binary name
TARGET  := seqops.exe

# Source and object files
SRCS    := main.c cli.c sequence.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean

## Default target: build binary and auto-clean objects
all: $(TARGET)
	@echo Build complete. Cleaning up object files...
	-@$(MAKE) clean-objects

## Link rule
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

## Generic .c -> .o rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

## Clean all build artifacts (objects + executable)
clean:
	-@echo Cleaning all build files...
	-@del /Q *.o 2>nul || true
	-@del /Q $(TARGET) 2>nul || true
	@echo Clean done.

## Clean only object files (used after build)
clean-objects:
	-@del /Q *.o 2>nul || true
