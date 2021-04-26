AS = as
LD = ld
CC = gcc
OBJDUMP = objdump
OBJCOPY = objcopy
SIZE = size
CFLAGS = -O2 -Wall -c -I ./src


all:

all: asgard-viking32 asgard-viking16 asgard-viking16-32 asgard-mars asgard-x86_linux

asgard-mars: asgard asgard-mars.o
asgard-mars.o: src/frontend/asgard.c codegen/mars/gen.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../../codegen/mars/gen.c\" -o $@

asgard-viking32: asgard asgard-viking32.o
asgard-viking32.o: src/frontend/asgard.c codegen/viking/gen32.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../../codegen/viking/gen32.c\" -o $@

asgard-viking16: asgard asgard-viking16.o
asgard-viking16.o: src/frontend/asgard.c codegen/viking/gen16.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../../codegen/viking/gen16.c\" -o $@

asgard-viking16-32: asgard asgard-viking16-32.o
asgard-viking16-32.o: src/frontend/asgard.c codegen/viking/gen16-32.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../../codegen/viking/gen16-32.c\" -o $@

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
	rm -f asgard-viking32 asgard-viking16 asgard-viking16-32 asgard-mars asgard-x86 out out.s *.txt *.lst
	rm -f *.o *~ *.elf *.bin src/*~ tests/*~

.PHONY: all
