# Directories
SRC_DIR := src
OBJ_DIR := obj

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
# Object files
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Flags
CC := gcc
CFLAGS := -I./include -g -O2 -march=native -mtune=native
LDFLAGS := 

# Executable
TARGET := fat32_emulator

# Default target
all: $(TARGET)

# Create obj directory if it doesn't exist
$(OBJ_DIR):	
	mkdir -p $(OBJ_DIR)

# Compilation
$(OBJ_DIR)/%.o:	$(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

# Linking 
$(TARGET):	$(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Include dependency files
-include $(OBJS:.o=.d)

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(OBJS:.o=.d)

# Phony targets
.PHONY: all clean
