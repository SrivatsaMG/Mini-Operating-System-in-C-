#include "kernel/kernel.hpp"
#include <iostream>
#include <thread>
#include <csignal>

using namespace MiniOS;

static std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", initiating shutdown...\n";
    g_running = false;
    Kernel::instance().halt();
}

void demonstrateScheduler(Kernel& kernel) {
    std::cout << "\n=== Scheduler Demonstration ===\n";
    
    kernel.getScheduler().createTask("worker1", []() {
        LOG_INFO("worker1", "Executing work...");
    }, TaskPriority::Normal);
    
    kernel.getScheduler().createTask("worker2", []() {
        LOG_INFO("worker2", "Processing data...");
    }, TaskPriority::High);
    
    kernel.getScheduler().createTask("background", []() {
        LOG_INFO("background", "Background task running...");
    }, TaskPriority::Low);
    
    for (int i = 0; i < 5; ++i) {
        kernel.getScheduler().schedule();
        kernel.getScheduler().tick();
    }
    
    kernel.getScheduler().printTaskStates();
}

void demonstrateMemory(Kernel& kernel) {
    std::cout << "\n=== Memory Management Demonstration ===\n";
    
    TaskId testTask = 100;
    kernel.getMemoryManager().createAddressSpace(testTask);
    
    auto page1 = kernel.getMemoryManager().allocatePage(testTask, 0, MemoryProtection::ReadWrite);
    auto page2 = kernel.getMemoryManager().allocatePage(testTask, 1, MemoryProtection::Read);
    auto page3 = kernel.getMemoryManager().allocatePage(testTask, 2, MemoryProtection::ReadWrite);
    
    if (page1) LOG_INFO("Demo", "Allocated page 0");
    if (page2) LOG_INFO("Demo", "Allocated page 1");
    if (page3) LOG_INFO("Demo", "Allocated page 2");
    
    kernel.getMemoryManager().printMemoryMap(testTask);
    std::cout << kernel.getMemoryManager().getMemoryReport();
    
    kernel.getMemoryManager().freePage(testTask, 1);
    kernel.getMemoryManager().destroyAddressSpace(testTask);
}

void demonstrateFileSystem(Kernel& kernel) {
    std::cout << "\n=== File System Demonstration ===\n";
    
    kernel.getFileSystem().createDirectory("/home", 0);
    kernel.getFileSystem().createDirectory("/home/user", 0);
    kernel.getFileSystem().createFile("/home/user/hello.txt", 0);
    
    auto fd = kernel.getFileSystem().open("/home/user/hello.txt", 
                                           OpenMode::ReadWrite | OpenMode::Create, 0);
    
    if (fd != INVALID_FD) {
        const char* message = "Hello from MiniOS!";
        kernel.getFileSystem().write(fd, message, strlen(message));
        
        kernel.getFileSystem().seek(fd, 0);
        
        char buffer[256] = {0};
        auto bytesRead = kernel.getFileSystem().read(fd, buffer, sizeof(buffer));
        
        LOG_INFO("Demo", "Read from file: " + std::string(buffer, bytesRead));
        
        kernel.getFileSystem().close(fd);
    }
    
    std::cout << "\nDirectory Tree:\n";
    kernel.getFileSystem().printDirectoryTree("/");
    
    std::cout << kernel.getFileSystem().getFileSystemReport();
}

void demonstrateIPC(Kernel& kernel) {
    std::cout << "\n=== IPC Demonstration ===\n";
    
    TaskId sender = 1;
    TaskId receiver = 2;
    
    kernel.getIPCManager().registerTask(sender);
    kernel.getIPCManager().registerTask(receiver);
    
    struct TestMessage {
        int type;
        int value;
        char text[32];
    };
    
    TestMessage msg1 = {1, 42, "Hello from sender!"};
    kernel.getIPCManager().sendMessage(sender, receiver, &msg1, sizeof(msg1));
    
    TestMessage msg2 = {2, 100, "Second message"};
    kernel.getIPCManager().sendAsync(sender, receiver, &msg2, sizeof(msg2));
    
    LOG_INFO("Demo", "Messages pending for receiver: " + 
             std::to_string(kernel.getIPCManager().getMessageCount(receiver)));
    
    while (auto msg = kernel.getIPCManager().receiveMessage(receiver, false)) {
        if (auto data = msg->getData<TestMessage>()) {
            LOG_INFO("Demo", "Received: type=" + std::to_string(data->type) + 
                     ", value=" + std::to_string(data->value) + 
                     ", text=" + std::string(data->text));
        }
    }
    
    std::cout << kernel.getIPCManager().getIPCReport();
}

void demonstrateDrivers(Kernel& kernel) {
    std::cout << "\n=== Driver Demonstration ===\n";
    
    auto* keyboard = dynamic_cast<KeyboardDriver*>(
        kernel.getDriverManager().getDriver("keyboard"));
    
    if (keyboard) {
        keyboard->simulateKeySequence("MiniOS>");
        
        char buffer[64] = {0};
        auto bytesRead = keyboard->read(buffer, sizeof(buffer));
        
        LOG_INFO("Demo", "Read from keyboard: " + std::string(buffer, bytesRead));
    }
    
    auto* timer = dynamic_cast<TimerDriver*>(
        kernel.getDriverManager().getDriver("timer"));
    
    if (timer) {
        LOG_INFO("Demo", "Timer tick count: " + std::to_string(timer->getTickCount()));
        LOG_INFO("Demo", "Elapsed time: " + std::to_string(timer->getElapsedMs()) + " ms");
    }
    
    std::cout << kernel.getDriverManager().getDriverReport();
    std::cout << kernel.getInterruptController().getInterruptReport();
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    Logger::instance().setLevel(LogLevel::Info);
    
    std::cout << R"(
  __  __ _       _ ___  ____  
 |  \/  (_)_ __ (_) _ \/ ___| 
 | |\/| | | '_ \| | | | \___ \ 
 | |  | | | | | | | |_| |___) |
 |_|  |_|_|_| |_|_|\___/|____/ 
                              
    Mini Microkernel Operating System
    Educational Implementation in C++17
    
)" << std::endl;

    Kernel& kernel = Kernel::instance();
    
    if (!kernel.boot()) {
        std::cerr << "Failed to boot kernel!\n";
        return 1;
    }
    
    demonstrateScheduler(kernel);
    demonstrateMemory(kernel);
    demonstrateFileSystem(kernel);
    demonstrateIPC(kernel);
    demonstrateDrivers(kernel);
    
    std::cout << kernel.getKernelReport();
    
    std::cout << "\n=== Running Main Loop (press Ctrl+C to exit) ===\n";
    std::cout << "The kernel is now running. Simulating time slices...\n\n";
    
    std::thread kernelThread([&kernel]() {
        kernel.run();
    });
    
    for (int i = 0; i < 10 && g_running; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        LOG_INFO("Main", "System running... tick " + std::to_string(i + 1));
    }
    
    kernel.shutdown();
    
    if (kernelThread.joinable()) {
        kernelThread.join();
    }
    
    Logger::instance().dumpToFile("kernel.log");
    
    std::cout << "\n=== MiniOS Terminated ===\n";
    return 0;
}
