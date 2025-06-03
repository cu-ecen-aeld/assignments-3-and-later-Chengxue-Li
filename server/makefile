# Variables
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g
TARGET = aesdsocket
SRC = aesdsocket.c
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Build the writer application
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(TARGET) $(OBJ)

# Phony targets
.PHONY: all clean