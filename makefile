.PHONY: build run clean

NAME := corescape
EMU := x64sc
CC := oscar64
CFLAGS = -dVSPRITES_MAX=24 -dNUM_IRQS=20 -dZPAGE_IRQS -xz -Oz -g -O2
PRG := $(NAME).prg
SRC := $(wildcard *.cpp)
ASM := $(wildcard *.asm)
MAIN_SRC := $(NAME).cpp

build: $(PRG)

run: $(PRG)
	$(EMU) $<

clean:
	rm -f $(PRG) $(ASM)

$(PRG): $(SRC)
	$(CC) $(CFLAGS) $(MAIN_SRC)
