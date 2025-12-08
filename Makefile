# Root Makefile - build ASM + C + run (PIC-ready)

ASM_DIR=asm
C_DIR=c
EXEC=test_main

all: $(EXEC)

# Step 1: build ASM library
$(ASM_DIR)/libasmoria.a:
	$(MAKE) -C $(ASM_DIR)

# Step 2: compile C code and link ASM library with PIC
$(EXEC): $(C_DIR)/main.c $(C_DIR)/idle_core.c $(ASM_DIR)/libasmoria.a
	$(CC) -Wall -g -o $(EXEC) $(C_DIR)/main.c $(C_DIR)/idle_core.c $(ASM_DIR)/libasmoria.a -no-pie

# Step 3: run program
run: $(EXEC)
	./$(EXEC)

# Clean all
clean:
	$(MAKE) -C $(ASM_DIR) clean
	rm -f $(EXEC)
