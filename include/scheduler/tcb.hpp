#pragma once

#include "kernel/types.hpp"
#include <string>
#include <chrono>

namespace MiniOS {

struct TaskControlBlock {
    TaskId id;
    std::string name;
    TaskState state;
    TaskPriority priority;
    CPUContext context;
    
    size_t stackSize;
    std::unique_ptr<uint8_t[]> stack;
    
    TaskId parentId;
    std::vector<TaskId> children;
    
    std::chrono::steady_clock::time_point creationTime;
    std::chrono::steady_clock::time_point lastScheduledTime;
    uint64_t cpuTimeMs;
    uint32_t timeSliceRemaining;
    
    size_t memoryUsage;
    std::vector<PageNumber> allocatedPages;
    
    std::vector<FileDescriptor> openFiles;
    
    int exitCode;

    TaskControlBlock(TaskId taskId, const std::string& taskName, TaskPriority prio = TaskPriority::Normal)
        : id(taskId)
        , name(taskName)
        , state(TaskState::Created)
        , priority(prio)
        , stackSize(PAGE_SIZE * 4)
        , parentId(INVALID_TASK_ID)
        , creationTime(std::chrono::steady_clock::now())
        , lastScheduledTime(creationTime)
        , cpuTimeMs(0)
        , timeSliceRemaining(TIME_QUANTUM_MS)
        , memoryUsage(0)
        , exitCode(0)
    {
        stack = std::make_unique<uint8_t[]>(stackSize);
        context.stackPointer = reinterpret_cast<uint64_t>(stack.get() + stackSize);
    }

    std::string stateToString() const {
        switch (state) {
            case TaskState::Created: return "Created";
            case TaskState::Ready: return "Ready";
            case TaskState::Running: return "Running";
            case TaskState::Blocked: return "Blocked";
            case TaskState::Waiting: return "Waiting";
            case TaskState::Terminated: return "Terminated";
            default: return "Unknown";
        }
    }

    std::string priorityToString() const {
        switch (priority) {
            case TaskPriority::Idle: return "Idle";
            case TaskPriority::Low: return "Low";
            case TaskPriority::Normal: return "Normal";
            case TaskPriority::High: return "High";
            case TaskPriority::RealTime: return "RealTime";
            default: return "Unknown";
        }
    }
};

}
