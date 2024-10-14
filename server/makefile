# compiler and flags
CC ?= gcc
CFLAGS = -g -Wall -Werror

# cross-compile
CROSS_COMPILE ?= 
CC := $(CROSS_COMPILE)gcc

# target executable
TARGET = aesdsocket

# source files
SRC = aesdsocket.c

# default target
all: $(TARGET)

# rule for target executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# clean rule
clean:
	rm -f $(TARGET)
