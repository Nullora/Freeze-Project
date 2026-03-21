AS = gcc
CC = gcc
LD = ld
CFLAGS = -ffreestanding -m32 -Wall -Wextra
LDFLAGS = -m elf_i386
BUILDDIR = FreezeProject/build

all: freeze.iso

$(BUILDDIR)/start.o: FreezeProject/start.S
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c FreezeProject/start.S -o $(BUILDDIR)/start.o

$(BUILDDIR)/kernel.o: FreezeProject/kernel.c
	$(CC) $(CFLAGS) -c FreezeProject/kernel.c -o $(BUILDDIR)/kernel.o

$(BUILDDIR)/crt.o: FreezeProject/crt.c
	$(CC) $(CFLAGS) -c FreezeProject/crt.c -o $(BUILDDIR)/crt.o

$(BUILDDIR)/serial.o: FreezeProject/serial.c
	$(CC) $(CFLAGS) -c FreezeProject/serial.c -o $(BUILDDIR)/serial.o

$(BUILDDIR)/input.o: FreezeProject/input.c
	$(CC) $(CFLAGS) -c FreezeProject/input.c -o $(BUILDDIR)/input.o

$(BUILDDIR)/memory.o: FreezeProject/memory.c
	$(CC) $(CFLAGS) -c FreezeProject/memory.c -o $(BUILDDIR)/memory.o

$(BUILDDIR)/rtc.o: FreezeProject/rtc.c
	$(CC) $(CFLAGS) -c FreezeProject/rtc.c -o $(BUILDDIR)/rtc.o

$(BUILDDIR)/shell.o: FreezeProject/shell.c
	$(CC) $(CFLAGS) -c FreezeProject/shell.c -o $(BUILDDIR)/shell.o

$(BUILDDIR)/timer.o: FreezeProject/timer.c
	$(CC) $(CFLAGS) -c FreezeProject/timer.c -o $(BUILDDIR)/timer.o

$(BUILDDIR)/vga.o: FreezeProject/vga.c
	$(CC) $(CFLAGS) -c FreezeProject/vga.c -o $(BUILDDIR)/vga.o

$(BUILDDIR)/kernel.bin: $(BUILDDIR)/start.o $(BUILDDIR)/crt.o $(BUILDDIR)/serial.o $(BUILDDIR)/kernel.o $(BUILDDIR)/input.o $(BUILDDIR)/memory.o $(BUILDDIR)/rtc.o $(BUILDDIR)/shell.o $(BUILDDIR)/timer.o $(BUILDDIR)/vga.o FreezeProject/linker.ld
	$(LD) $(LDFLAGS) -T FreezeProject/linker.ld -o $(BUILDDIR)/kernel.bin $(BUILDDIR)/start.o $(BUILDDIR)/crt.o $(BUILDDIR)/serial.o $(BUILDDIR)/kernel.o $(BUILDDIR)/input.o $(BUILDDIR)/memory.o $(BUILDDIR)/rtc.o $(BUILDDIR)/shell.o $(BUILDDIR)/timer.o $(BUILDDIR)/vga.o

iso/boot/grub/grub.cfg: FreezeProject/grub/grub.cfg
	mkdir -p iso/boot/grub
	cp FreezeProject/grub/grub.cfg iso/boot/grub/

freeze.iso: $(BUILDDIR)/kernel.bin iso/boot/grub/grub.cfg
	rm -rf iso/boot/kernel.bin
	mkdir -p iso/boot/grub
	cp $(BUILDDIR)/kernel.bin iso/boot/
	grub-mkrescue -o freeze.iso iso

clean:
	rm -rf $(BUILDDIR) freeze.iso
	rm -rf iso

.PHONY: all clean