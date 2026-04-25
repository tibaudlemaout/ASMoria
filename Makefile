# =========================================================
# ASMoria Makefile
# Toolchain: NASM + GCC + SDL2 + SDL_ttf (Ubuntu / WSL2)
#
# Targets:
#   make          - build the game binary
#   make run      - build and run
#   make clean    - remove all build artifacts
#   make install-deps - install required apt packages
# =========================================================

# --- Compiler & assembler ---
CC      := gcc
NASM    := nasm

# --- Flags ---
CFLAGS  := -std=c11 -Wall -Wextra -O2 \
           -I include \
           $(shell sdl2-config --cflags)

LDFLAGS := $(shell sdl2-config --libs) \
           -lSDL2_ttf

NASMFLAGS := -f elf64 -I asm/

# --- Directories ---
SRCDIR  := src
ASMDIR  := asm
BLDDIR  := build

# --- Source files ---
C_SRCS  := $(SRCDIR)/main.c              \
            $(SRCDIR)/render/renderer.c  \
            $(SRCDIR)/ui/ui.c            \
            $(SRCDIR)/game/game.c

ASM_SRCS := $(ASMDIR)/core/tick.asm      \
             $(ASMDIR)/core/resources.asm \
             $(ASMDIR)/math/rng.asm

# --- Object files ---
C_OBJS   := $(patsubst $(SRCDIR)/%.c,   $(BLDDIR)/%.c.o,   $(C_SRCS))
ASM_OBJS := $(patsubst $(ASMDIR)/%.asm, $(BLDDIR)/%.asm.o, $(ASM_SRCS))

# --- Binary ---
TARGET := $(BLDDIR)/asmoria

# =========================================================
# Default target
# =========================================================
.PHONY: all
all: $(TARGET)

$(TARGET): $(C_OBJS) $(ASM_OBJS)
	@echo "[LD]  $@"
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo ""
	@echo "  Build complete: $(TARGET)"
	@echo "  Run with: make run"

# =========================================================
# C compilation rules
# =========================================================
$(BLDDIR)/%.c.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC]  $<"
	$(CC) $(CFLAGS) -c $< -o $@

# =========================================================
# NASM assembly rules
# =========================================================
$(BLDDIR)/%.asm.o: $(ASMDIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "[AS]  $<"
	$(NASM) $(NASMFLAGS) $< -o $@

# =========================================================
# Convenience targets
# =========================================================
.PHONY: run
run: all
	./$(TARGET)

.PHONY: clean
clean:
	rm -rf $(BLDDIR)
	@echo "Cleaned."

.PHONY: install-deps
install-deps:
	sudo apt update
	sudo apt install -y \
		nasm \
		gcc \
		libsdl2-dev \
		libsdl2-ttf-dev \
		libsdl2-image-dev

.PHONY: info
info:
	@echo "C sources:   $(C_SRCS)"
	@echo "ASM sources: $(ASM_SRCS)"
	@echo "C objects:   $(C_OBJS)"
	@echo "ASM objects: $(ASM_OBJS)"
	@echo "Target:      $(TARGET)"