# compiler and flags
CROSS_COMPILE ?= 
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?=

# target executable
TARGET ?= aesdsocket

# source files
SRCS = aesdsocket.c

# Define the object files
OBJS ?= $(SRCS:.c=.o)

# default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Rule to compile the source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean rule
clean:
	rm -f $(TARGET) $(OBJS)
