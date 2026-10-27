# Smart-Blood-Bank-Management-System
# RaktSetu Mini OS — Kernel Prototype

## Overview

RaktSetu Mini OS is a 32-bit freestanding kernel prototype developed as part of the RaktSetu Blood Bank Management System project. It boots through GRUB and runs in QEMU, providing a text-based terminal for demonstrating blood-bank resource management and selected operating-system algorithms.

The kernel prototype extends the project's simulator concept into a bootable environment. It includes a kernel-owned blood inventory, a process table, an FCFS scheduling demonstration, a first-fit memory allocation demonstration, and an FCFS disk scheduling demonstration.

> **Project scope:** This is an educational kernel prototype. The scheduling, memory allocation, and disk scheduling features demonstrate algorithms using in-memory data structures. They do not yet implement full hardware resource management or real multitasking.

## Implemented Features

### 1. Bootable Kernel and Terminal
- 32-bit x86 kernel entry point using NASM assembly.
- Multiboot-compatible kernel loaded through GRUB.
- Kernel execution tested in QEMU.
- VGA text-mode output and an interactive command-line interface.
- Keyboard input handled through polling.

### 2. Blood Bank Inventory Management
- Tracks stock for eight blood groups: A+, A−, B+, B−, AB+, AB−, O+, and O−.
- Displays the current stock using the `blood` command.
- Accepts blood requests using `request GROUP UNITS`.
- Updates inventory when sufficient stock is available.
- Rejects requests when the requested quantity exceeds available stock.
- Keeps inventory in RAM; stock resets when the kernel restarts.

### 3. Process Management
- Maintains a fixed-size process table.
- Assigns process IDs (PIDs) to registered processes.
- Stores process names and states.
- Lists registered processes through the `ps` command.
- Demonstrates READY-to-DONE state transitions through the FCFS scheduling routine.

### 4. FCFS CPU Scheduling Demonstration
- Processes ready entries in insertion order.
- Displays the execution order of registered processes.
- Updates process records to reflect completion in the scheduling demonstration.

**Limitation:** The current scheduler demonstrates ordering and state changes; it does not perform real CPU context switching or preemptive multitasking.

### 5. First-Fit Memory Allocation Demonstration
- Uses a fixed set of simulated memory blocks.
- Scans blocks in order and selects the first free block large enough for a request.
- Displays block sizes and allocation status.
- Reports allocation success or failure.
- Provides a reset command for repeating the demonstration.

**Limitation:** This is a simulated block allocator, not a physical RAM allocator. Blocks are allocated as whole units; block splitting and coalescing are not implemented.

### 6. FCFS Disk Scheduling Demonstration
- Uses a predefined queue of disk-cylinder requests.
- Processes requests in first-come, first-served order.
- Calculates total disk-head movement for the request sequence.
- Displays the request order and total movement.

**Limitation:** This is an algorithm demonstration using fixed input data, not a physical disk driver.

## Command Reference

| Command | Description |
|---|---|
| `help` | Lists available commands |
| `about` | Displays kernel information |
| `status` | Displays kernel and subsystem status |
| `clear` | Clears the terminal |
| `blood` | Displays blood inventory |
| `request O+ 2` | Requests two units of O+ blood |
| `proc add Demo` | Registers a process named Demo |
| `ps` | Lists process IDs, names, and states |
| `schedule` | Runs the FCFS scheduling demonstration |
| `mem` | Displays simulated memory blocks |
| `mem alloc 180` | Requests a memory block of size 180 |
| `mem reset` | Resets the memory demonstration |
| `disk` | Runs the FCFS disk scheduling demonstration |

## Build and Run

### Prerequisites

- Ubuntu or WSL
- GCC with 32-bit compilation support
- NASM
- GNU linker
- GRUB utilities, including `grub-file` and `grub-mkrescue`
- `xorriso`
- QEMU system emulator

### Compile the kernel

From the `kernel/` directory:

```bash
nasm -f elf32 boot/boot.s -o boot.o

gcc -m32 -ffreestanding -fno-stack-protector \
    -fno-pic -fno-pie -mno-sse -mno-mmx -msoft-float \
    -Wall -Wextra -c src/kernel.c -o kernel.o

ld -m elf_i386 -T linker.ld -nostdlib \
    boot.o kernel.o -o kernel.elf

grub-file --is-x86-multiboot kernel.elf
```

### Create the bootable ISO

```bash
cp kernel.elf iso/boot/kernel.elf
grub-mkrescue -o raktsetu-kernel.iso iso
```

### Run in QEMU

```bash
qemu-system-i386 -cdrom raktsetu-kernel.iso
```

## Demonstration Workflow

The following sequence demonstrates the main implemented features:

```text
help
status
blood
request O+ 2
blood
request O- 10

proc add Demo
proc add Inventory
ps
schedule
ps

mem
mem alloc 180
mem
mem alloc 450
mem reset

disk
```

Expected behavior:
- O+ stock decreases by two after a successful request.
- A request for ten units of O− is rejected when only five are available.
- The process table displays registered PIDs and states.
- FCFS scheduling processes ready entries in insertion order.
- First-fit allocation selects the first available block large enough for each request.
- FCFS disk scheduling displays its fixed request order and total head movement.

## Current Limitations and Future Work

The current prototype does not yet implement:
- Real CPU context switching and independent process execution.
- GDT/IDT-based interrupt handling, PIC remapping, or a PIT timer.
- An interrupt-driven keyboard driver.
- Synchronization primitives integrated into the kernel.
- System calls such as `int 0x80`.
- Physical memory management, block splitting/coalescing, or actual disk I/O.
- Persistent inventory storage across reboots.

Further development should focus on interrupt handling, genuine process execution and context switching, synchronization, system-call support, and integration testing with the existing simulator modules.

## Testing

The kernel has been manually tested in QEMU for booting, terminal commands, blood inventory operations, process-table operations, FCFS scheduling, first-fit allocation, and FCFS disk scheduling.

Automated regression testing, broader edge-case coverage, and integration with the complete simulator remain separate validation tasks.

## Project Goal

The project explores how operating-system concepts and algorithm demonstrations can support a blood-bank resource management application. The kernel prototype provides a bootable environment in which selected concepts can be demonstrated and extended incrementally.
