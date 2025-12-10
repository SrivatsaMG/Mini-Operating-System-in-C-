#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <variant>

namespace MiniOS {

using TaskId = uint32_t;
using ProcessId = uint32_t;
using PageNumber = uint32_t;
using FrameNumber = uint32_t;
using FileDescriptor = int32_t;
using MessageId = uint32_t;
using InterruptNumber = uint16_t;

constexpr TaskId INVALID_TASK_ID = 0xFFFFFFFF;
constexpr FileDescriptor INVALID_FD = -1;
constexpr size_t PAGE_SIZE = 4096;
constexpr size_t MAX_TASKS = 256;
constexpr size_t MAX_OPEN_FILES = 1024;
constexpr size_t TIME_QUANTUM_MS = 100;

enum class TaskState {
    Created,
    Ready,
    Running,
    Blocked,
    Waiting,
    Terminated
};

enum class TaskPriority {
    Idle = 0,
    Low = 1,
    Normal = 2,
    High = 3,
    RealTime = 4
};

enum class MemoryProtection : uint8_t {
    None = 0,
    Read = 1,
    Write = 2,
    Execute = 4,
    ReadWrite = Read | Write,
    ReadExecute = Read | Execute,
    All = Read | Write | Execute
};

inline MemoryProtection operator|(MemoryProtection a, MemoryProtection b) {
    return static_cast<MemoryProtection>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline MemoryProtection operator&(MemoryProtection a, MemoryProtection b) {
    return static_cast<MemoryProtection>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

enum class SystemCallId {
    Exit = 0,
    Fork = 1,
    Read = 2,
    Write = 3,
    Open = 4,
    Close = 5,
    Send = 6,
    Receive = 7,
    Allocate = 8,
    Free = 9,
    Yield = 10,
    Sleep = 11,
    GetPid = 12,
    CreateTask = 13
};

struct CPUContext {
    uint64_t registers[16];
    uint64_t programCounter;
    uint64_t stackPointer;
    uint64_t flags;
    
    CPUContext() : programCounter(0), stackPointer(0), flags(0) {
        for (auto& reg : registers) reg = 0;
    }
};

}
