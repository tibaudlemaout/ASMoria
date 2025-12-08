# Root Makefile - auto-detect C files and link ASM library

CC = gcc
CFLAGS = -Wall -g -no-pie

ASM_DIR = asm
C_DIR = c
EXEC = test_main

# Automatically detect all .c files in c/
C_SOURCES := $(wildcard $(C_DIR)/*.c)
C_OBJECTS := $(C_SOURCES:.c=.o)

all: $(EXEC)

# Step 1: build the ASM library
$(ASM_DIR)/libasmoria.a:
	$(MAKE) -C $(ASM_DIR)

# Step 2: build C object files
$(C_DIR)/%.o: $(C_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Step 3: link everything into the final executable
$(EXEC): $(ASM_DIR)/libasmoria.a $(C_OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC) $(C_OBJECTS) $(ASM_DIR)/libasmoria.a

# Step 4: run program
run: $(EXEC)
	./$(EXEC)

clean:
	$(MAKE) -C $(ASM_DIR) clean
	rm -f $(C_OBJECTS) $(EXEC)

re: clean all run
