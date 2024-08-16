# compiler and flags
CC ?= gcc
CFLAGS = -Wall -g

# cross-compile
CROSS_COMPILE ?= 
CC := $(CROSS_COMPILE)gcc

# target executable
TARGET = writer

# source files
SRC = writer.c

# default target
all: $(TARGET)

# rule for target executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# clean rule
clean:
	rm -f $(TARGET)
