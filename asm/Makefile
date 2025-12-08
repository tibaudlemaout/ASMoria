# ASM Library Makefile
ASM=nasm
ASMFLAGS=-f elf64

# Output library
LIB=libasmoria.a
OBJ=core.o resource.o

all: $(LIB)

# Build static library
$(LIB): $(OBJ)
	ar rcs $(LIB) $(OBJ)

# Compile ASM files
core.o: core.asm
	$(ASM) $(ASMFLAGS) -o core.o core.asm

resource.o: resource.asm
	$(ASM) $(ASMFLAGS) -o resource.o resource.asm

clean:
	rm -f *.o *.a
