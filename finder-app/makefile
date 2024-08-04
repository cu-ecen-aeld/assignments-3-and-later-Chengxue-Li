# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror

# Cross-compiler setup
ifdef CROSS_COMPILE
    CC = $(CROSS_COMPILE)gcc
endif

# Targets
TARGET = writer
OBJS = writer.o

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Object file generation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
