AS = as
LD = ld
CC = gcc
OBJDUMP = objdump
OBJCOPY = objcopy
SIZE = size
CFLAGS = -O2 -Wall

all: asgard-viking32 asgard-viking16 asgard-viking16-32 asgard-mars asgard-x86_linux

asgard-mars: asgard-mars.o
asgard-mars.o: src/asgard.c codegen/mars/gen.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../codegen/mars/gen.c\" -o $@

asgard-viking32: asgard-viking32.o
asgard-viking32.o: src/asgard.c codegen/viking/gen32.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../codegen/viking/gen32.c\" -o $@

asgard-viking16: asgard-viking16.o
asgard-viking16.o: src/asgard.c codegen/viking/gen16.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../codegen/viking/gen16.c\" -o $@

asgard-viking16-32: asgard-viking16-32.o
asgard-viking16-32.o: src/asgard.c codegen/viking/gen16-32.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../codegen/viking/gen16-32.c\" -o $@

asgard-x86_linux: asgard-x86_linux.o
asgard-x86_linux.o: src/asgard.c codegen/x86_linux/gen.c
	$(CC) $(CFLAGS) -c $< -DCODEGEN=\"../codegen/x86_linux/gen.c\" -o $@

app_x86_linux:
	$(AS) out.s -o out.o --32
	$(LD) -m elf_i386 -o out.elf out.o
#	$(LD) -m elf_i386 -s -o out out.o	# this will strip the binary
	$(OBJCOPY) -O binary out.elf out.bin
	$(OBJDUMP) -d -s out.elf > out.lst
	$(SIZE) out.elf

clean:
	rm -f asgard-viking32 asgard-viking16 asgard-viking16-32 asgard-mars asgard-x86_linux out out.s *.txt *.lst
	rm -f *.o *~ *.elf *.bin src/*~ tests/*~

.PHONY: all

