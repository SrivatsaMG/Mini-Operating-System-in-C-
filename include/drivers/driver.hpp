#pragma once

#include "kernel/types.hpp"
#include "utils/logger.hpp"
#include <map>
#include <vector>
#include <functional>
#include <queue>
#include <memory>

namespace MiniOS {

enum class InterruptType {
    Timer = 0,
    Keyboard = 1,
    Disk = 2,
    Network = 3,
    SystemCall = 128,
    PageFault = 14,
    GeneralProtection = 13
};

using InterruptHandler = std::function<void(InterruptNumber, void*)>;

struct InterruptDescriptor {
    InterruptNumber number;
    InterruptHandler handler;
    std::string name;
    uint64_t triggerCount;
    bool enabled;

    InterruptDescriptor(InterruptNumber num, InterruptHandler h, const std::string& n)
        : number(num), handler(std::move(h)), name(n), triggerCount(0), enabled(true) {}
};

class InterruptController {
public:
    InterruptController();
    ~InterruptController() = default;

    bool registerHandler(InterruptNumber interrupt, InterruptHandler handler, const std::string& name);
    bool unregisterHandler(InterruptNumber interrupt);
    
    void triggerInterrupt(InterruptNumber interrupt, void* data = nullptr);
    
    void enableInterrupt(InterruptNumber interrupt);
    void disableInterrupt(InterruptNumber interrupt);
    bool isEnabled(InterruptNumber interrupt) const;

    void enableInterrupts();
    void disableInterrupts();
    bool areInterruptsEnabled() const { return interruptsEnabled_; }

    std::string getInterruptReport() const;

private:
    std::map<InterruptNumber, InterruptDescriptor> handlers_;
    bool interruptsEnabled_;
    uint64_t totalInterrupts_;
};

enum class DriverType {
    Character,
    Block,
    Network
};

class Driver {
public:
    Driver(const std::string& name, DriverType type);
    virtual ~Driver() = default;

    virtual bool init() = 0;
    virtual bool shutdown() = 0;
    virtual ssize_t read(void* buffer, size_t count) = 0;
    virtual ssize_t write(const void* buffer, size_t count) = 0;
    virtual bool ioctl(uint32_t command, void* arg) = 0;

    const std::string& getName() const { return name_; }
    DriverType getType() const { return type_; }
    bool isInitialized() const { return initialized_; }

protected:
    std::string name_;
    DriverType type_;
    bool initialized_;
};

class KeyboardDriver : public Driver {
public:
    KeyboardDriver();
    ~KeyboardDriver() override = default;

    bool init() override;
    bool shutdown() override;
    ssize_t read(void* buffer, size_t count) override;
    ssize_t write(const void* buffer, size_t count) override;
    bool ioctl(uint32_t command, void* arg) override;

    void simulateKeyPress(char key);
    void simulateKeySequence(const std::string& sequence);
    bool hasInput() const { return !inputBuffer_.empty(); }

private:
    std::queue<char> inputBuffer_;
    bool echoEnabled_;
    static constexpr size_t BUFFER_SIZE = 256;
};

class TimerDriver : public Driver {
public:
    TimerDriver();
    ~TimerDriver() override = default;

    bool init() override;
    bool shutdown() override;
    ssize_t read(void* buffer, size_t count) override;
    ssize_t write(const void* buffer, size_t count) override;
    bool ioctl(uint32_t command, void* arg) override;

    void tick();
    uint64_t getTickCount() const { return tickCount_; }
    uint64_t getElapsedMs() const;
    void setFrequency(uint32_t hz);

private:
    uint64_t tickCount_;
    uint32_t frequency_;
    std::chrono::steady_clock::time_point startTime_;
};

class DriverManager {
public:
    DriverManager();
    ~DriverManager() = default;

    bool registerDriver(std::unique_ptr<Driver> driver);
    bool unregisterDriver(const std::string& name);
    
    Driver* getDriver(const std::string& name);
    std::vector<std::string> getDriverList() const;

    bool initAllDrivers();
    bool shutdownAllDrivers();

    std::string getDriverReport() const;

private:
    std::map<std::string, std::unique_ptr<Driver>> drivers_;
};

}
