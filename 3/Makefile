# ==========================================
# DSP Sequence Operations (Windows-friendly Makefile)
# ==========================================

# --- Compiler & flags ---
CC = gcc
CFLAGS = -std=c11 -O2 -Iinclude
DEBUGFLAGS = -std=c11 -Wall -Wextra -g -Og -Iinclude

# --- Directories ---
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# --- Target ---
TARGET = $(BIN_DIR)/dsp_seq.exe

# --- Source and object files ---
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# --- Default rule ---
all: dirs $(TARGET)
	@echo Build complete! Executable: $(TARGET)

# --- Link step ---
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

# --- Compile step ---
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Create directories if not exist ---
dirs:
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"

# --- Debug build ---
debug: CFLAGS = $(DEBUGFLAGS)
debug: clean all
	@echo Debug build complete!

# --- Clean ---
clean:
	@if exist "$(OBJ_DIR)" rmdir /S /Q "$(OBJ_DIR)"
	@if exist "$(BIN_DIR)" rmdir /S /Q "$(BIN_DIR)"
	@echo Clean complete.

# --- Phony targets ---
.PHONY: all clean debug dirs
