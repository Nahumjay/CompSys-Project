CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = main

# The default target to build
all: $(TARGET)  

# Target to compile main.c into the main executable
$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

# Clean up any generated files
clean:
	rm -f $(TARGET)
