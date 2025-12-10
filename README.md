# MiniOS - Mini Microkernel Operating System

A lightweight, educational microkernel operating system implementation written in modern C++ (C++17). This project demonstrates fundamental OS concepts including process scheduling, memory management, file systems, inter-process communication, and device drivers.

## Overview

MiniOS is designed as a learning tool for understanding operating system internals. It implements a microkernel architecture where core kernel services are minimal, and most OS functionality runs as separate modules that could theoretically run in user space.

### Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Applications                         │
├─────────────────────────────────────────────────────────────────┤
│     File System     │      IPC       │     Device Drivers       │
│                     │    Manager     │                          │
├─────────────────────┴────────────────┴──────────────────────────┤
│                      Memory Manager                              │
├─────────────────────────────────────────────────────────────────┤
│                         Scheduler                                │
├─────────────────────────────────────────────────────────────────┤
│                      Kernel Core                                 │
│              (Interrupt Handling, System Calls)                  │
└─────────────────────────────────────────────────────────────────┘
```

## Features

### 1. Kernel Core
- Microkernel architecture with minimal kernel functionality
- System call interface for user-space to kernel communication
- Kernel panic handling and graceful shutdown
- Comprehensive kernel logging and reporting

### 2. Task Scheduling
- **Round-Robin Scheduler**: Fair time-sharing between tasks
- **Priority-Based Scheduler**: Tasks scheduled by priority level
- Task Control Block (TCB) with full context information
- Simulated context switching
- Time slice management with configurable quantum

### 3. Memory Management
- Virtual memory simulation with page tables
- Physical frame allocation using bitmap
- Page fault handling and demand paging
- Memory protection flags (Read/Write/Execute)
- Heap allocator with malloc/free semantics
- Memory coalescing for efficient allocation

### 4. File System
- In-memory file system with inode structure
- Directory hierarchy support
- File operations: create, read, write, delete, seek
- File descriptor table management
- Path normalization and traversal

### 5. Inter-Process Communication (IPC)
- Message passing between tasks
- Synchronous and asynchronous messaging
- Message queues per task
- Multiple message types (Data, Signal, Request, Response, Notification)
- Send-and-wait-reply pattern support

### 6. Device Drivers
- Driver abstraction with common interface
- Keyboard driver (simulated input)
- Timer driver with configurable frequency
- Interrupt controller with handler registration
- Driver manager for registration and lifecycle

### 7. Debugging & Logging
- Multi-level logging (Debug, Info, Warning, Error, Critical)
- Per-component log tagging
- Log history and file export
- Kernel state reporting
- Memory usage visualization

## Project Structure

```
MiniOS/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── include/                    # Header files
│   ├── kernel/
│   │   ├── kernel.hpp          # Main kernel interface
│   │   └── types.hpp           # Common type definitions
│   ├── scheduler/
│   │   ├── scheduler.hpp       # Scheduler interface
│   │   └── tcb.hpp             # Task Control Block
│   ├── mm/
│   │   └── memory_manager.hpp  # Memory management
│   ├── fs/
│   │   └── filesystem.hpp      # File system
│   ├── ipc/
│   │   └── ipc.hpp             # IPC mechanisms
│   ├── drivers/
│   │   └── driver.hpp          # Device drivers
│   └── utils/
│       └── logger.hpp          # Logging utilities
├── src/                        # Implementation files
│   ├── kernel/
│   │   └── kernel.cpp
│   ├── scheduler/
│   │   └── scheduler.cpp
│   ├── mm/
│   │   └── memory_manager.cpp
│   ├── fs/
│   │   └── filesystem.cpp
│   ├── ipc/
│   │   └── ipc.cpp
│   ├── drivers/
│   │   └── driver.cpp
│   └── main.cpp                # Entry point and demos
└── tests/                      # Unit tests
    ├── test_scheduler.cpp
    ├── test_memory.cpp
    ├── test_filesystem.cpp
    └── test_ipc.cpp
```

## Building

### Prerequisites
- CMake 3.16 or higher
- C++17 compatible compiler (Clang 14+, GCC 8+, MSVC 2019+)
- pthread library

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
make

# Run the OS
./minios

# Run tests
make test
# or
ctest --output-on-failure
```

### Build Options

```bash
# Debug build (default)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Usage

### Running MiniOS

```bash
./minios
```

The program will:
1. Boot the kernel and initialize all subsystems
2. Demonstrate each OS component (scheduler, memory, filesystem, IPC, drivers)
3. Print a comprehensive kernel report
4. Run the main kernel loop (press Ctrl+C to exit)
5. Gracefully shutdown and save logs

### Sample Output

```
  __  __ _       _ ___  ____  
 |  \/  (_)_ __ (_) _ \/ ___| 
 | |\/| | | '_ \| | | | \___ \ 
 | |  | | | | | | | |_| |___) |
 |_|  |_|_|_| |_|_|\___/|____/ 
                              
    Mini Microkernel Operating System
    Educational Implementation in C++17

[INFO] [Kernel] Starting boot sequence...
[INFO] [Kernel] Initializing subsystems...
...
```

## API Examples

### Creating Tasks

```cpp
#include "kernel/kernel.hpp"

auto& kernel = MiniOS::Kernel::instance();
kernel.boot();

// Create a task with normal priority
kernel.getScheduler().createTask("worker", []() {
    // Task code here
}, MiniOS::TaskPriority::Normal);
```

### Memory Allocation

```cpp
auto& mm = kernel.getMemoryManager();

// Create address space for a task
mm.createAddressSpace(taskId);

// Allocate a page
auto page = mm.allocatePage(taskId, 0, MiniOS::MemoryProtection::ReadWrite);
```

### File Operations

```cpp
auto& fs = kernel.getFileSystem();

// Create and write to a file
fs.createFile("/data.txt", taskId);
auto fd = fs.open("/data.txt", MiniOS::OpenMode::ReadWrite, taskId);
fs.write(fd, "Hello", 5);
fs.close(fd);
```

### Inter-Process Communication

```cpp
auto& ipc = kernel.getIPCManager();

// Register tasks
ipc.registerTask(senderId);
ipc.registerTask(receiverId);

// Send a message
int data = 42;
ipc.sendMessage(senderId, receiverId, &data, sizeof(data));

// Receive the message
auto msg = ipc.receiveMessage(receiverId);
```

## Testing

Unit tests are provided for each major component:

```bash
# Run all tests
cd build && ctest

# Run individual tests
./test_scheduler
./test_memory
./test_filesystem
./test_ipc
```

## Design Decisions

1. **Microkernel Architecture**: Core kernel is minimal; services run as separate modules
2. **Simulation Focus**: This is an educational simulator, not a real bootable OS
3. **Modern C++**: Uses C++17 features like std::optional, structured bindings, etc.
4. **No External Dependencies**: Only uses C++ STL for portability
5. **Thread Safety**: Uses mutexes where needed for IPC and logging

## Limitations

- This is a simulation, not a real bootable operating system
- Does not interface with actual hardware
- No real context switching (simulated)
- In-memory file system (no persistence)
- Single process space (no true process isolation)

## Contributing

Contributions are welcome! Areas for improvement:
- Add more scheduling algorithms (MLFQ, CFS)
- Implement virtual memory swapping
- Add network stack simulation
- Create a shell interface
- Add more device drivers

## License

This project is open source and available for educational purposes.

## Author

Educational project for learning operating system concepts.
