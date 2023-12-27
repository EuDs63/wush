CC = gcc
# -Wall: 启用大多数的警告信息；-Wextra: 启用一些额外的警告；-g: 在生成的可执行文件中包含调试信息
CFLAGS = -Wall -Wextra -g
TARGET = wush
SRC_DIR = ./src
BUILD_DIR = build

# List of source files
SRCS = $(wildcard $(SRC_DIR)/*.c)

# List of object files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Dependency files
DEPS = $(OBJS:.o=.d)

# Main target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

# Phony target to clean up the build directory
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Include dependency files
-include $(DEPS)

# Create build directory
$(shell mkdir -p $(BUILD_DIR))
