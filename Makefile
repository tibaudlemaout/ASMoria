# =========================================================
# ASMoria Makefile
# =========================================================

CC        := gcc
NASM      := nasm

CFLAGS    := -std=c11 -Wall -Wextra -O2 \
             -I include \
             $(shell sdl2-config --cflags)

LDFLAGS   := $(shell sdl2-config --libs) -lSDL2_ttf

NASMFLAGS := -f elf64 -I asm/

SRCDIR    := src
ASMDIR    := asm
BLDDIR    := build

C_SRCS    := $(SRCDIR)/main.c              \
             $(SRCDIR)/render/renderer.c   \
             $(SRCDIR)/ui/ui.c             \
             $(SRCDIR)/ui/ui_upgrades.c    \
             $(SRCDIR)/game/game.c \
             $(SRCDIR)/game/save.c

ASM_SRCS  := $(ASMDIR)/core/tick.asm      \
             $(ASMDIR)/core/resources.asm  \
             $(ASMDIR)/core/events.asm     \
             $(ASMDIR)/core/dwarves.asm    \
             $(ASMDIR)/core/hire.asm       \
             $(ASMDIR)/core/jobs.asm       \
             $(ASMDIR)/core/upgrades.asm   \
             $(ASMDIR)/core/save.asm       \
             $(ASMDIR)/core/stat_events.asm \
             $(ASMDIR)/core/xp.asm           \
             $(ASMDIR)/core/names.asm       \
             $(ASMDIR)/math/rng.asm

C_OBJS    := $(patsubst $(SRCDIR)/%.c,   $(BLDDIR)/%.c.o,   $(C_SRCS))
ASM_OBJS  := $(patsubst $(ASMDIR)/%.asm, $(BLDDIR)/%.asm.o, $(ASM_SRCS))

TARGET    := $(BLDDIR)/asmoria

.PHONY: all
all: $(TARGET)

$(TARGET): $(C_OBJS) $(ASM_OBJS)
	@echo "[LD]  $@"
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "  Build complete -> $(TARGET)"

$(BLDDIR)/%.c.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC]  $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BLDDIR)/%.asm.o: $(ASMDIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "[AS]  $<"
	$(NASM) $(NASMFLAGS) $< -o $@

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
	sudo apt install -y nasm gcc libsdl2-dev libsdl2-ttf-dev

.PHONY: info
info:
	@echo "C sources:   $(C_SRCS)"
	@echo "ASM sources: $(ASM_SRCS)"
	@echo "Target:      $(TARGET)"