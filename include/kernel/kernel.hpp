#pragma once

#include "kernel/types.hpp"
#include "scheduler/scheduler.hpp"
#include "mm/memory_manager.hpp"
#include "fs/filesystem.hpp"
#include "ipc/ipc.hpp"
#include "drivers/driver.hpp"
#include "utils/logger.hpp"
#include <memory>
#include <atomic>
#include <chrono>

namespace MiniOS {

enum class KernelState {
    Uninitialized,
    Booting,
    Running,
    Halting,
    Halted
};

class SystemCall {
public:
    static int64_t dispatch(SystemCallId id, uint64_t arg1 = 0, 
                           uint64_t arg2 = 0, uint64_t arg3 = 0);
};

class Kernel {
public:
    static Kernel& instance() {
        static Kernel kernel;
        return kernel;
    }

    bool boot();
    void run();
    void halt();
    void shutdown();

    Scheduler& getScheduler() { return *scheduler_; }
    MemoryManager& getMemoryManager() { return *memoryManager_; }
    FileSystem& getFileSystem() { return *fileSystem_; }
    IPCManager& getIPCManager() { return *ipcManager_; }
    DriverManager& getDriverManager() { return *driverManager_; }
    InterruptController& getInterruptController() { return *interruptController_; }

    KernelState getState() const { return state_; }
    uint64_t getUptime() const;
    std::string getSystemInfo() const;
    std::string getKernelReport() const;

    void panic(const std::string& message);

private:
    Kernel();
    ~Kernel();
    
    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;

    bool initSubsystems();
    void setupInterruptHandlers();
    void createIdleTask();
    void mainLoop();
    void handleTimerInterrupt(InterruptNumber num, void* data);

    std::unique_ptr<Scheduler> scheduler_;
    std::unique_ptr<MemoryManager> memoryManager_;
    std::unique_ptr<FileSystem> fileSystem_;
    std::unique_ptr<IPCManager> ipcManager_;
    std::unique_ptr<DriverManager> driverManager_;
    std::unique_ptr<InterruptController> interruptController_;

    KernelState state_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point bootTime_;
    uint64_t tickCount_;

    static constexpr const char* VERSION = "0.1.0";
    static constexpr const char* NAME = "MiniOS";
};

#define SYSCALL(id, ...) MiniOS::SystemCall::dispatch(MiniOS::SystemCallId::id, ##__VA_ARGS__)

}
