# Virtual File System Simulator (C++)

## Overview

This project is an **in-memory virtual file system** implemented in modern C++.  
It simulates a Unix-like hierarchical filesystem with sector-based storage, directory trees, and file management operations.

The goal of the project is to demonstrate understanding of:

- filesystem architecture
- memory management
- tree-based data structures
- command-line interface design
- disk allocation strategies

The system behaves like a minimal operating system shell for managing files and directories.

---

## Key Features

- Hierarchical directory structure
- Sector-based virtual disk simulation
- File creation, deletion, copy, and move
- Recursive directory operations
- Path resolution (`.`, `..`, absolute & relative paths)
- Disk defragmentation algorithm
- Real ↔ virtual file transfer
- Metadata inspection (size, sectors, path)
- Interactive CLI shell

---

## Architecture

The filesystem is built around three core components:

### 1. Virtual Disk Layer

- Fixed-size sectors
- Sector allocation bitmap
- Fragmentation simulation
- Defragmentation support

Each file is stored as chunks mapped to disk sectors.

### 2. File Tree Structure

- Tree-based directory model
- Each node represents a file or folder
- Parent-child relationships
- Recursive traversal algorithms

This mimics inode-style organization used in real filesystems.

### 3. Command Shell

Interactive CLI that supports:

```
pwd
cd
ls
mkdir
touch
rm
cp
mv
get
put
info
defrag
```

The shell parses user input and dispatches filesystem operations.

---

## Project Structure

```
.
├── main.cpp
└── README.md
```

---

## Build

Requires a C++17 compatible compiler.

```bash
g++ -std=c++17 main.cpp -o vfs
```

---

## Run

```bash
./vfs
```

You will be prompted to define disk capacity (number of sectors).

---

## Example Session

```
fs:$ mkdir docs
fs:$ touch notes.txt
fs:$ ls
docs/
notes.txt
fs:$ info notes.txt
```

---

## Technical Highlights

- Manual memory management with recursive cleanup
- Sector allocation algorithm with reuse
- Fragmentation + defragmentation simulation
- Path parser with edge case handling
- Recursive copy/move of directory trees
- Error-safe CLI with exception handling
- OOP design with encapsulated filesystem logic

---

## Future Improvements

- Persistent disk serialization
- Permission system
- Journaling / crash recovery simulation
- Multi-user support
- Block caching
- Performance benchmarking
- Unit testing framework

---

## Skills Demonstrated

- C++ OOP design
- Data structures (trees, vectors, maps)
- Systems programming concepts
- Algorithm design
- Memory lifecycle management
- CLI tooling
- Software architecture
