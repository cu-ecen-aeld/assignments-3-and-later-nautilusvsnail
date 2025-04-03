# compiler and flags
CROSS_COMPILE ?= 
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror

# target executable
TARGET ?= aesdsocket

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
