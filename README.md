# Operating-Systems-programming

A collection of C programs exploring core Operating Systems concepts, including **process management** and **FAT32 filesystem parsing** on Linux.

---

## Repository Structure

```
Operating-Systems-programming/
│
├── Lab 1 – Process Management & GCC Pipeline
│   ├── code_tp.c       # Base version: compile and run a C program via child processes
│   └── pg.c            # Extended version: adds signal-based abnormal termination handling
│
├── Lab 2 – FAT32 Filesystem Analysis
│   ├── Part1.c         # Read GPT partition table and FAT32 boot sector metadata
│   └── Part2.c         # Extended version: also lists root directory contents
│
└── README.md
```

---

## Lab 1 — Process Management & GCC Pipeline

### Description

This lab demonstrates Unix **process creation and management** using `fork()` and `exec()`. The program simulates a mini build system: it asks the user for a C source file, object file name, and executable name, then orchestrates three child processes to compile and run the program step by step.

**Process flow:**

```
Parent
  └── P1 (fork) → gcc -c source.c -o object.o      [compile to object file]
        └── P2 (fork) → gcc object.o -o executable  [link to executable]
              └── P3 (fork) → ./executable           [run the program]
```

Each child process is waited on before the next is spawned. Exit status is inspected via `WIFEXITED`, `WEXITSTATUS`, and in the extended version (`pg.c`), `WIFSIGNALED` and `WIFSTOPPED` to detect signal-based termination.

### Files

| File | Description |
|------|-------------|
| `code_tp.c` | Base implementation — detects normal vs abnormal termination |
| `pg.c` | Extended version — also catches `WIFSIGNALED` and `WTERMSIG`/`WIFSTOPPED`/`WSTOPSIG` for P3 |

### Key Concepts

- `fork()` — create child processes
- `execlp()` — replace a process image with a new program
- `wait()` / `WIFEXITED()` / `WEXITSTATUS()` — synchronize and inspect child exit status
- `WIFSIGNALED()` / `WTERMSIG()` / `WIFSTOPPED()` / `WSTOPSIG()` — handle abnormal termination

### How to Build & Run

```bash
gcc code_tp.c -o code_tp
./code_tp
```

When prompted:
```
Veuillez entrer le nom du fichier source (***.c) : hello.c
Veuillez entrer le nom du fichier objet (***.o) : hello.o
Veuillez entrer le nom du fichier exécutable : hello
```

> **Note:** The target source file (e.g. `hello.c`) must exist in the current directory before running.

---

## Lab 2 — FAT32 Filesystem Analysis

### Description

This lab involves low-level **disk and filesystem parsing** in C. The programs read raw sectors from a block device, parse a **GPT (GUID Partition Table)** to locate a FAT32 partition, then decode the **FAT32 boot sector** to extract filesystem metadata.

Part 2 goes further by traversing the **root directory** and listing all files with their sizes, cluster numbers, and LBA addresses.

### Files

| File | Description |
|------|-------------|
| `Part1.c` | Reads GPT entries, locates the FAT32 partition, and prints boot sector info |
| `Part2.c` | Extends Part 1 — also prints a formatted table of root directory contents |

### Key Concepts

- **MBR & GPT parsing** — reading sector 0 (protective MBR) and sector 1 (GPT header) to locate partition entries
- **FAT32 Boot Sector** — decoding BPB fields: bytes per sector, sectors per cluster, reserved sectors, FAT count, FAT size, root cluster
- **LBA addressing** — computing the logical block address of the root directory from boot sector parameters
- **Directory entry parsing** — reading 32-byte FAT32 directory entries to extract file names, sizes, and first cluster numbers
- **`/dev` enumeration** — listing available block devices (`sd*`, `hd*`) by scanning `/dev`

### Output Example (Part 2)

```
LBA début de la partition FAT32: 2048
Taille en secteurs de la partition FAT32: 204800
Numéro du premier cluster du répertoire racine: 2
LBA début du répertoire racine: 8284
Taille d'un cluster en secteurs: 8

    ******************* Contenu du répertoire racine *******************

|-----------|-----------------------|------------------|-----------------|---------------------|
| Nom       |    Taille(secteurs)   |  Taille Ko       | Premier cluster |  LBA premier Cluster|
|-----------|-----------------------|------------------|-----------------|---------------------|
| HELLO      |                     1 |             0.50 |               3 |                8292 |
...
```

### How to Build & Run

> ⚠️ **Root privileges are required** to read raw block devices.

```bash
gcc Part1.c -o part1
sudo ./part1
```

```bash
gcc Part2.c -o part2
sudo ./part2
```

By default the programs target `/dev/sdb2` (Part2) or `/dev/sda` (Part1). Edit the `disque` variable in `main()` to match your target device before compiling.

---

## Requirements

- Linux (tested on Ubuntu)
- GCC
- Root access (for Lab 2 disk reading)

---

## Notes

- Lab 2 assumes a **GPT-partitioned disk** where the FAT32 partition is the **second GPT entry** (index 1). Adjust the pointer offset in `main()` if your layout differs.
- All disk reads use raw `fopen`/`fread` at the sector level (512 bytes per sector).
- Lab 1 is entirely self-contained and safe to run on any Linux system without elevated privileges.
