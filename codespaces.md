# codespace guide

This will guide explains how to build and run the Freeze OS project inside **GitHub Codespaces** using QEMU (non graphic).

---

## 1. Install Required Packages

Open the VS Code terminal inside Codespaces and run:

```bash
sudo apt update
sudo apt install build-essential grub-pc-bin grub-common xorriso qemu-system-x86 -y
```

## 2. Build the Project
From the root of the repo:
```bash
make && make run
```

if you want you may check for the iso file:
```bash
ls
```

## run the OS in qemu
We mainly use nographic for qemu so run:
```bash
qemu-system-i386 -cdrom freeze.iso -nographic -serial mon:stdio
```

> Thanks for listening
