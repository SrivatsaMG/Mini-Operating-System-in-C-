#include "kernel/kernel.hpp"
#include <sstream>
#include <iomanip>
#include <thread>

namespace MiniOS {

Kernel::Kernel()
    : state_(KernelState::Uninitialized)
    , running_(false)
    , tickCount_(0)
{
}

Kernel::~Kernel() {
    if (state_ == KernelState::Running) {
        shutdown();
    }
}

bool Kernel::boot() {
    if (state_ != KernelState::Uninitialized) {
        LOG_ERROR("Kernel", "Kernel already booted");
        return false;
    }
    
    state_ = KernelState::Booting;
    bootTime_ = std::chrono::steady_clock::now();
    
    LOG_INFO("Kernel", "========================================");
    LOG_INFO("Kernel", "  " + std::string(NAME) + " v" + std::string(VERSION));
    LOG_INFO("Kernel", "  Mini Microkernel Operating System");
    LOG_INFO("Kernel", "========================================");
    LOG_INFO("Kernel", "Starting boot sequence...");
    
    if (!initSubsystems()) {
        LOG_CRITICAL("Kernel", "Failed to initialize subsystems");
        state_ = KernelState::Halted;
        return false;
    }
    
    setupInterruptHandlers();
    createIdleTask();
    
    state_ = KernelState::Running;
    running_ = true;
    
    LOG_INFO("Kernel", "Boot complete. System ready.");
    LOG_INFO("Kernel", "========================================");
    
    return true;
}

void Kernel::run() {
    if (state_ != KernelState::Running) {
        LOG_ERROR("Kernel", "Cannot run: kernel not in running state");
        return;
    }
    
    LOG_INFO("Kernel", "Entering main kernel loop");
    interruptController_->enableInterrupts();
    
    mainLoop();
}

void Kernel::halt() {
    LOG_INFO("Kernel", "Halting kernel...");
    state_ = KernelState::Halting;
    running_ = false;
}

void Kernel::shutdown() {
    LOG_INFO("Kernel", "Shutting down...");
    
    halt();
    
    interruptController_->disableInterrupts();
    
    driverManager_->shutdownAllDrivers();
    
    LOG_INFO("Kernel", "Shutdown complete");
    state_ = KernelState::Halted;
}

uint64_t Kernel::getUptime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - bootTime_).count();
}

std::string Kernel::getSystemInfo() const {
    std::stringstream ss;
    ss << NAME << " v" << VERSION << "\n";
    ss << "State: ";
    switch (state_) {
        case KernelState::Uninitialized: ss << "Uninitialized"; break;
        case KernelState::Booting: ss << "Booting"; break;
        case KernelState::Running: ss << "Running"; break;
        case KernelState::Halting: ss << "Halting"; break;
        case KernelState::Halted: ss << "Halted"; break;
    }
    ss << "\n";
    ss << "Uptime: " << getUptime() << " ms\n";
    ss << "Tick Count: " << tickCount_ << "\n";
    return ss.str();
}

std::string Kernel::getKernelReport() const {
    std::stringstream ss;
    ss << "\n";
    ss << "╔══════════════════════════════════════════════════════════════╗\n";
    ss << "║                    MINIOS KERNEL REPORT                      ║\n";
    ss << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    ss << getSystemInfo() << "\n";
    ss << scheduler_->getTaskReport() << "\n";
    ss << memoryManager_->getMemoryReport() << "\n";
    ss << fileSystem_->getFileSystemReport() << "\n";
    ss << ipcManager_->getIPCReport() << "\n";
    ss << driverManager_->getDriverReport() << "\n";
    ss << interruptController_->getInterruptReport() << "\n";
    
    return ss.str();
}

void Kernel::panic(const std::string& message) {
    LOG_CRITICAL("Kernel", "!!! KERNEL PANIC !!!");
    LOG_CRITICAL("Kernel", message);
    
    interruptController_->disableInterrupts();
    
    std::cerr << "\n*** KERNEL PANIC ***\n" << message << "\n";
    std::cerr << "System halted.\n";
    
    state_ = KernelState::Halted;
    running_ = false;
}

bool Kernel::initSubsystems() {
    LOG_INFO("Kernel", "Initializing subsystems...");
    
    LOG_INFO("Kernel", "  -> Scheduler");
    scheduler_ = std::make_unique<Scheduler>(SchedulerType::RoundRobin);
    
    LOG_INFO("Kernel", "  -> Memory Manager");
    memoryManager_ = std::make_unique<MemoryManager>();
    
    LOG_INFO("Kernel", "  -> File System");
    fileSystem_ = std::make_unique<FileSystem>();
    
    LOG_INFO("Kernel", "  -> IPC Manager");
    ipcManager_ = std::make_unique<IPCManager>();
    
    LOG_INFO("Kernel", "  -> Interrupt Controller");
    interruptController_ = std::make_unique<InterruptController>();
    
    LOG_INFO("Kernel", "  -> Driver Manager");
    driverManager_ = std::make_unique<DriverManager>();
    
    auto timerDriver = std::make_unique<TimerDriver>();
    auto keyboardDriver = std::make_unique<KeyboardDriver>();
    
    driverManager_->registerDriver(std::move(timerDriver));
    driverManager_->registerDriver(std::move(keyboardDriver));
    driverManager_->initAllDrivers();
    
    LOG_INFO("Kernel", "All subsystems initialized successfully");
    return true;
}

void Kernel::setupInterruptHandlers() {
    LOG_INFO("Kernel", "Setting up interrupt handlers...");
    
    interruptController_->registerHandler(
        static_cast<InterruptNumber>(InterruptType::Timer),
        [this](InterruptNumber num, void* data) {
            handleTimerInterrupt(num, data);
        },
        "Timer"
    );
    
    interruptController_->registerHandler(
        static_cast<InterruptNumber>(InterruptType::Keyboard),
        [](InterruptNumber num, void* data) {
            LOG_DEBUG("Kernel", "Keyboard interrupt received");
        },
        "Keyboard"
    );
    
    interruptController_->registerHandler(
        static_cast<InterruptNumber>(InterruptType::SystemCall),
        [](InterruptNumber num, void* data) {
            LOG_DEBUG("Kernel", "System call interrupt");
        },
        "SystemCall"
    );
    
    interruptController_->registerHandler(
        static_cast<InterruptNumber>(InterruptType::PageFault),
        [this](InterruptNumber num, void* data) {
            LOG_WARN("Kernel", "Page fault occurred");
        },
        "PageFault"
    );
    
    LOG_INFO("Kernel", "Interrupt handlers configured");
}

void Kernel::createIdleTask() {
    LOG_INFO("Kernel", "Creating idle task...");
    
    scheduler_->createTask("idle", []() {
    }, TaskPriority::Idle);
    
    ipcManager_->registerTask(0);
    memoryManager_->createAddressSpace(0);
}

void Kernel::mainLoop() {
    const auto tickInterval = std::chrono::milliseconds(TIME_QUANTUM_MS);
    auto nextTick = std::chrono::steady_clock::now();
    
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        
        if (now >= nextTick) {
            tickCount_++;
            
            auto* timerDriver = dynamic_cast<TimerDriver*>(driverManager_->getDriver("timer"));
            if (timerDriver) {
                timerDriver->tick();
            }
            
            interruptController_->triggerInterrupt(static_cast<InterruptNumber>(InterruptType::Timer));
            
            nextTick = now + tickInterval;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Kernel::handleTimerInterrupt(InterruptNumber num, void* data) {
    scheduler_->tick();
}

int64_t SystemCall::dispatch(SystemCallId id, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    Kernel& kernel = Kernel::instance();
    
    switch (id) {
        case SystemCallId::Exit:
            LOG_DEBUG("Syscall", "Exit called with code " + std::to_string(arg1));
            {
                auto* task = kernel.getScheduler().getCurrentTask();
                if (task) {
                    kernel.getScheduler().terminateTask(task->id);
                }
            }
            return 0;
            
        case SystemCallId::Yield:
            kernel.getScheduler().yield();
            return 0;
            
        case SystemCallId::GetPid:
            {
                auto* task = kernel.getScheduler().getCurrentTask();
                return task ? task->id : -1;
            }
            
        case SystemCallId::Allocate:
            {
                auto* task = kernel.getScheduler().getCurrentTask();
                if (task) {
                    auto result = kernel.getMemoryManager().allocatePage(
                        task->id, 
                        static_cast<PageNumber>(arg1)
                    );
                    return result ? reinterpret_cast<int64_t>(*result) : -1;
                }
            }
            return -1;
            
        case SystemCallId::Free:
            {
                auto* task = kernel.getScheduler().getCurrentTask();
                if (task) {
                    return kernel.getMemoryManager().freePage(task->id, static_cast<PageNumber>(arg1)) ? 0 : -1;
                }
            }
            return -1;
            
        case SystemCallId::Send:
            {
                auto* task = kernel.getScheduler().getCurrentTask();
                if (task) {
                    return kernel.getIPCManager().sendMessage(
                        task->id,
                        static_cast<TaskId>(arg1),
                        reinterpret_cast<void*>(arg2),
                        static_cast<size_t>(arg3)
                    );
                }
            }
            return -1;
            
        case SystemCallId::Open:
            {
                auto* task = kernel.getScheduler().getCurrentTask();
                if (task) {
                    return kernel.getFileSystem().open(
                        reinterpret_cast<const char*>(arg1),
                        static_cast<OpenMode>(arg2),
                        task->id
                    );
                }
            }
            return -1;
            
        case SystemCallId::Close:
            return kernel.getFileSystem().close(static_cast<FileDescriptor>(arg1)) ? 0 : -1;
            
        case SystemCallId::Read:
            return kernel.getFileSystem().read(
                static_cast<FileDescriptor>(arg1),
                reinterpret_cast<void*>(arg2),
                static_cast<size_t>(arg3)
            );
            
        case SystemCallId::Write:
            return kernel.getFileSystem().write(
                static_cast<FileDescriptor>(arg1),
                reinterpret_cast<const void*>(arg2),
                static_cast<size_t>(arg3)
            );
            
        default:
            LOG_WARN("Syscall", "Unknown system call: " + std::to_string(static_cast<int>(id)));
            return -1;
    }
}

}
