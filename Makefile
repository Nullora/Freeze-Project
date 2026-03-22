AS = gcc
CC = gcc
LD = ld
CFLAGS = -ffreestanding -m32 -Wall -Wextra -IFreezeProject/include -IFreezeProject/src
LDFLAGS = -m elf_i386
P = FreezeProject
BUILDDIR = FreezeProject/build
SRCDIR = FreezeProject/src

all: freeze.iso

$(BUILDDIR)/start.o: $(SRCDIR)/start.S
	mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $(SRCDIR)/start.S -o $(BUILDDIR)/start.o

$(BUILDDIR)/kernel.o: $(SRCDIR)/kernel.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/kernel.c -o $(BUILDDIR)/kernel.o

$(BUILDDIR)/crt.o: $(SRCDIR)/crt.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/crt.c -o $(BUILDDIR)/crt.o

$(BUILDDIR)/serial.o: $(SRCDIR)/serial.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/serial.c -o $(BUILDDIR)/serial.o

$(BUILDDIR)/input.o: $(SRCDIR)/input.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/input.c -o $(BUILDDIR)/input.o

$(BUILDDIR)/memory.o: $(SRCDIR)/memory.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/memory.c -o $(BUILDDIR)/memory.o

$(BUILDDIR)/rtc.o: $(SRCDIR)/rtc.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/rtc.c -o $(BUILDDIR)/rtc.o

$(BUILDDIR)/shell.o: $(SRCDIR)/shell.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/shell.c -o $(BUILDDIR)/shell.o

$(BUILDDIR)/timer.o: $(SRCDIR)/timer.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/timer.c -o $(BUILDDIR)/timer.o

$(BUILDDIR)/vga.o: $(SRCDIR)/vga.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/vga.c -o $(BUILDDIR)/vga.o

$(BUILDDIR)/kernel.bin: $(BUILDDIR)/start.o $(BUILDDIR)/crt.o $(BUILDDIR)/serial.o $(BUILDDIR)/kernel.o $(BUILDDIR)/input.o $(BUILDDIR)/memory.o $(BUILDDIR)/rtc.o $(BUILDDIR)/shell.o $(BUILDDIR)/timer.o $(BUILDDIR)/vga.o $(SRCDIR)/linker.ld
	$(LD) $(LDFLAGS) -T $(SRCDIR)/linker.ld -o $(BUILDDIR)/kernel.bin $(BUILDDIR)/start.o $(BUILDDIR)/crt.o $(BUILDDIR)/serial.o $(BUILDDIR)/kernel.o $(BUILDDIR)/input.o $(BUILDDIR)/memory.o $(BUILDDIR)/rtc.o $(BUILDDIR)/shell.o $(BUILDDIR)/timer.o $(BUILDDIR)/vga.o

iso/boot/grub/grub.cfg: $(P)/grub/grub.cfg
	mkdir -p iso/boot/grub
	cp $(P)/grub/grub.cfg iso/boot/grub/

freeze.iso: $(BUILDDIR)/kernel.bin iso/boot/grub/grub.cfg
	rm -rf iso/boot/kernel.bin
	mkdir -p iso/boot/grub
	cp $(BUILDDIR)/kernel.bin iso/boot/
	grub-mkrescue -o freeze.iso iso

run: freeze.iso
	qemu-system-i386 -cdrom freeze.iso -nographic

clean:
	rm -rf $(BUILDDIR) freeze.iso
	rm -rf iso

.PHONY: all clean run