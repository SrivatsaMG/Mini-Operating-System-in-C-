# MiniOS - Mini Microkernel Operating System

## Project Overview

MiniOS is an educational microkernel operating system implementation written in modern C++ (C++17). It simulates core OS concepts including:

- Task scheduling (Round-Robin and Priority-based)
- Virtual memory management with page tables
- In-memory file system
- Inter-process communication (IPC)
- Device drivers (keyboard, timer)

## Project Structure

```
MiniOS/
├── CMakeLists.txt          # CMake build configuration
├── include/                # Header files
│   ├── kernel/             # Kernel core (types.hpp, kernel.hpp)
│   ├── scheduler/          # Scheduler (scheduler.hpp, tcb.hpp)
│   ├── mm/                 # Memory manager (memory_manager.hpp)
│   ├── fs/                 # File system (filesystem.hpp)
│   ├── ipc/                # IPC (ipc.hpp)
│   ├── drivers/            # Drivers (driver.hpp)
│   └── utils/              # Utilities (logger.hpp)
├── src/                    # Implementation files
│   ├── kernel/             # kernel.cpp
│   ├── scheduler/          # scheduler.cpp
│   ├── mm/                 # memory_manager.cpp
│   ├── fs/                 # filesystem.cpp
│   ├── ipc/                # ipc.cpp
│   ├── drivers/            # driver.cpp
│   └── main.cpp            # Entry point
└── tests/                  # Unit tests
```

## Technology Stack

- **Language**: C++17
- **Build System**: CMake 3.16+
- **Compiler**: Clang 14
- **Dependencies**: pthread (POSIX threads)

## Running the Project

### Build and Run
```bash
mkdir -p build && cd build
cmake ..
make
./minios
```

### Run Tests
```bash
cd build
make test
```

## Key Components

1. **Kernel**: Main kernel loop, system calls, interrupt handling
2. **Scheduler**: Round-Robin and Priority scheduling with TCB
3. **Memory Manager**: Virtual memory with page tables, heap allocator
4. **File System**: In-memory FS with inodes, directories, file descriptors
5. **IPC Manager**: Message passing between tasks
6. **Driver Manager**: Keyboard and timer drivers

## User Preferences

- Focus on OS fundamentals, not UI
- Clean, modular, production-style architecture
- Modern C++17 features preferred
- Well-documented code with clear comments
- Prefer clarity over extreme optimization
