AS = as
LD = ld
CC = gcc
OBJDUMP = objdump
OBJCOPY = objcopy
SIZE = size
CFLAGS = -O2 -Wall -c -I ./src


all:

all: asgard-mars asgard-mars-smd asgard-viking asgard-viking32 asgard-viking16-32 asgard-x86

asgard-mars:
	$(CC) $(CFLAGS) -I ./codegen/mars -o error.o src/error.c
	$(CC) $(CFLAGS) -I ./codegen/mars -o lexer.o src/lexer.c
	$(CC) $(CFLAGS) -I ./codegen/mars -o symbol.o src/symbol.c
	$(CC) $(CFLAGS) -I ./codegen/mars -o parser.o src/parser.c
	$(CC) $(CFLAGS) -I ./codegen/mars -o emit.o src/emit.c
	$(CC) $(CFLAGS) -I ./codegen/mars -o gen.o codegen/mars/gen.c
	$(CC) $(CFLAGS) -I ./codegen/mars -o asgard.o src/asgard.c
	$(CC) error.o lexer.o symbol.o parser.o emit.o gen.o asgard.o -o asgard-mars
	
asgard-mars-smd:
	$(CC) $(CFLAGS) -I ./codegen/mars_swmuldiv -o error.o src/error.c
	$(CC) $(CFLAGS) -I ./codegen/mars_swmuldiv -o lexer.o src/lexer.c
	$(CC) $(CFLAGS) -I ./codegen/mars_swmuldiv -o symbol.o src/symbol.c
	$(CC) $(CFLAGS) -I ./codegen/mars_swmuldiv -o parser.o src/parser.c
	$(CC) $(CFLAGS) -I ./codegen/mars_swmuldiv -o emit.o src/emit.c
	$(CC) $(CFLAGS) -I ./codegen/mars_swmuldiv -o gen.o codegen/mars_swmuldiv/gen.c
	$(CC) $(CFLAGS) -I ./codegen/mars_swmuldiv -o asgard.o src/asgard.c
	$(CC) error.o lexer.o symbol.o parser.o emit.o gen.o asgard.o -o asgard-mars-smd

asgard-viking:
	$(CC) $(CFLAGS) -I ./codegen/viking -o error.o src/error.c
	$(CC) $(CFLAGS) -I ./codegen/viking -o lexer.o src/lexer.c
	$(CC) $(CFLAGS) -I ./codegen/viking -o symbol.o src/symbol.c
	$(CC) $(CFLAGS) -I ./codegen/viking -o parser.o src/parser.c
	$(CC) $(CFLAGS) -I ./codegen/viking -o emit.o src/emit.c
	$(CC) $(CFLAGS) -I ./codegen/viking -o gen.o codegen/viking/gen.c
	$(CC) $(CFLAGS) -I ./codegen/viking -o asgard.o src/asgard.c
	$(CC) error.o lexer.o symbol.o parser.o emit.o gen.o asgard.o -o asgard-viking

asgard-viking32:
	$(CC) $(CFLAGS) -I ./codegen/viking32 -o error.o src/error.c
	$(CC) $(CFLAGS) -I ./codegen/viking32 -o lexer.o src/lexer.c
	$(CC) $(CFLAGS) -I ./codegen/viking32 -o symbol.o src/symbol.c
	$(CC) $(CFLAGS) -I ./codegen/viking32 -o parser.o src/parser.c
	$(CC) $(CFLAGS) -I ./codegen/viking32 -o emit.o src/emit.c
	$(CC) $(CFLAGS) -I ./codegen/viking32 -o gen.o codegen/viking32/gen.c
	$(CC) $(CFLAGS) -I ./codegen/viking32 -o asgard.o src/asgard.c
	$(CC) error.o lexer.o symbol.o parser.o emit.o gen.o asgard.o -o asgard-viking32

asgard-viking16-32:
	$(CC) $(CFLAGS) -I ./codegen/viking16-32 -o error.o src/error.c
	$(CC) $(CFLAGS) -I ./codegen/viking16-32 -o lexer.o src/lexer.c
	$(CC) $(CFLAGS) -I ./codegen/viking16-32 -o symbol.o src/symbol.c
	$(CC) $(CFLAGS) -I ./codegen/viking16-32 -o parser.o src/parser.c
	$(CC) $(CFLAGS) -I ./codegen/viking16-32 -o emit.o src/emit.c
	$(CC) $(CFLAGS) -I ./codegen/viking16-32 -o gen.o codegen/viking16-32/gen.c
	$(CC) $(CFLAGS) -I ./codegen/viking16-32 -o asgard.o src/asgard.c
	$(CC) error.o lexer.o symbol.o parser.o emit.o gen.o asgard.o -o asgard-viking16-32

asgard-x86:
	$(CC) $(CFLAGS) -I ./codegen/x86_linux -o error.o src/error.c
	$(CC) $(CFLAGS) -I ./codegen/x86_linux -o lexer.o src/lexer.c
	$(CC) $(CFLAGS) -I ./codegen/x86_linux -o symbol.o src/symbol.c
	$(CC) $(CFLAGS) -I ./codegen/x86_linux -o parser.o src/parser.c
	$(CC) $(CFLAGS) -I ./codegen/x86_linux -o emit.o src/emit.c
	$(CC) $(CFLAGS) -I ./codegen/x86_linux -o gen.o codegen/x86_linux/gen.c
	$(CC) $(CFLAGS) -I ./codegen/x86_linux -o asgard.o src/asgard.c
	$(CC) error.o lexer.o symbol.o parser.o emit.o gen.o asgard.o -o asgard-x86

app_x86:
	$(AS) out.s -o out.o --32
	$(LD) -m elf_i386 -o out.elf out.o
#	$(LD) -m elf_i386 -s -o out out.o	# this will strip the binary
	$(OBJCOPY) -O binary out.elf out.bin
	$(OBJDUMP) -d -s out.elf > out.lst
	$(SIZE) out.elf

clean:
	rm -f asgard-mars asgard-mars-smd asgard-viking asgard-viking32 asgard-viking16-32 asgard-x86 out out.s *.txt *.lst
	rm -f *.o *~ *.elf *.bin src/*~ tests/*~

.PHONY: all
