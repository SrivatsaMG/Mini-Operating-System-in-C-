#include "drivers/driver.hpp"
#include <sstream>
#include <iomanip>

namespace MiniOS {

InterruptController::InterruptController()
    : interruptsEnabled_(false)
    , totalInterrupts_(0)
{
    LOG_INFO("InterruptController", "Initialized interrupt controller");
}

bool InterruptController::registerHandler(InterruptNumber interrupt, 
                                           InterruptHandler handler, 
                                           const std::string& name) {
    if (handlers_.find(interrupt) != handlers_.end()) {
        LOG_WARN("InterruptController", "Handler already registered for interrupt " + 
                 std::to_string(interrupt));
        return false;
    }
    
    handlers_.emplace(interrupt, InterruptDescriptor(interrupt, std::move(handler), name));
    LOG_INFO("InterruptController", "Registered handler '" + name + "' for interrupt " + 
             std::to_string(interrupt));
    return true;
}

bool InterruptController::unregisterHandler(InterruptNumber interrupt) {
    auto it = handlers_.find(interrupt);
    if (it == handlers_.end()) {
        return false;
    }
    
    LOG_INFO("InterruptController", "Unregistered handler for interrupt " + 
             std::to_string(interrupt));
    handlers_.erase(it);
    return true;
}

void InterruptController::triggerInterrupt(InterruptNumber interrupt, void* data) {
    if (!interruptsEnabled_) {
        LOG_DEBUG("InterruptController", "Interrupts disabled, ignoring interrupt " + 
                  std::to_string(interrupt));
        return;
    }
    
    auto it = handlers_.find(interrupt);
    if (it == handlers_.end()) {
        LOG_WARN("InterruptController", "No handler for interrupt " + std::to_string(interrupt));
        return;
    }
    
    if (!it->second.enabled) {
        return;
    }
    
    totalInterrupts_++;
    it->second.triggerCount++;
    
    LOG_DEBUG("InterruptController", "Triggering interrupt " + std::to_string(interrupt) + 
              " (" + it->second.name + ")");
    
    it->second.handler(interrupt, data);
}

void InterruptController::enableInterrupt(InterruptNumber interrupt) {
    auto it = handlers_.find(interrupt);
    if (it != handlers_.end()) {
        it->second.enabled = true;
    }
}

void InterruptController::disableInterrupt(InterruptNumber interrupt) {
    auto it = handlers_.find(interrupt);
    if (it != handlers_.end()) {
        it->second.enabled = false;
    }
}

bool InterruptController::isEnabled(InterruptNumber interrupt) const {
    auto it = handlers_.find(interrupt);
    if (it == handlers_.end()) {
        return false;
    }
    return it->second.enabled;
}

void InterruptController::enableInterrupts() {
    interruptsEnabled_ = true;
    LOG_INFO("InterruptController", "Interrupts enabled");
}

void InterruptController::disableInterrupts() {
    interruptsEnabled_ = false;
    LOG_INFO("InterruptController", "Interrupts disabled");
}

std::string InterruptController::getInterruptReport() const {
    std::stringstream ss;
    ss << "=== Interrupt Controller Report ===\n";
    ss << "Interrupts Enabled: " << (interruptsEnabled_ ? "Yes" : "No") << "\n";
    ss << "Total Interrupts Handled: " << totalInterrupts_ << "\n";
    ss << "Registered Handlers: " << handlers_.size() << "\n\n";
    
    ss << std::setw(8) << "IRQ" << " | "
       << std::setw(20) << "Name" << " | "
       << std::setw(8) << "Enabled" << " | "
       << "Count" << "\n";
    ss << std::string(55, '-') << "\n";
    
    for (const auto& [num, desc] : handlers_) {
        ss << std::setw(8) << num << " | "
           << std::setw(20) << desc.name << " | "
           << std::setw(8) << (desc.enabled ? "Yes" : "No") << " | "
           << desc.triggerCount << "\n";
    }
    
    return ss.str();
}

Driver::Driver(const std::string& name, DriverType type)
    : name_(name)
    , type_(type)
    , initialized_(false)
{
}

KeyboardDriver::KeyboardDriver()
    : Driver("keyboard", DriverType::Character)
    , echoEnabled_(true)
{
}

bool KeyboardDriver::init() {
    if (initialized_) {
        return false;
    }
    
    while (!inputBuffer_.empty()) {
        inputBuffer_.pop();
    }
    
    initialized_ = true;
    LOG_INFO("KeyboardDriver", "Keyboard driver initialized");
    return true;
}

bool KeyboardDriver::shutdown() {
    if (!initialized_) {
        return false;
    }
    
    while (!inputBuffer_.empty()) {
        inputBuffer_.pop();
    }
    
    initialized_ = false;
    LOG_INFO("KeyboardDriver", "Keyboard driver shut down");
    return true;
}

ssize_t KeyboardDriver::read(void* buffer, size_t count) {
    if (!initialized_ || !buffer) {
        return -1;
    }
    
    char* charBuffer = static_cast<char*>(buffer);
    size_t bytesRead = 0;
    
    while (bytesRead < count && !inputBuffer_.empty()) {
        charBuffer[bytesRead++] = inputBuffer_.front();
        inputBuffer_.pop();
    }
    
    return static_cast<ssize_t>(bytesRead);
}

ssize_t KeyboardDriver::write(const void* buffer, size_t count) {
    return -1;
}

bool KeyboardDriver::ioctl(uint32_t command, void* arg) {
    switch (command) {
        case 0:
            if (arg) {
                echoEnabled_ = *static_cast<bool*>(arg);
                return true;
            }
            break;
        case 1:
            while (!inputBuffer_.empty()) {
                inputBuffer_.pop();
            }
            return true;
    }
    return false;
}

void KeyboardDriver::simulateKeyPress(char key) {
    if (!initialized_) {
        return;
    }
    
    if (inputBuffer_.size() < BUFFER_SIZE) {
        inputBuffer_.push(key);
        
        if (echoEnabled_) {
            std::cout << key << std::flush;
        }
    }
}

void KeyboardDriver::simulateKeySequence(const std::string& sequence) {
    for (char c : sequence) {
        simulateKeyPress(c);
    }
}

TimerDriver::TimerDriver()
    : Driver("timer", DriverType::Character)
    , tickCount_(0)
    , frequency_(100)
    , startTime_(std::chrono::steady_clock::now())
{
}

bool TimerDriver::init() {
    if (initialized_) {
        return false;
    }
    
    tickCount_ = 0;
    startTime_ = std::chrono::steady_clock::now();
    initialized_ = true;
    
    LOG_INFO("TimerDriver", "Timer driver initialized at " + std::to_string(frequency_) + " Hz");
    return true;
}

bool TimerDriver::shutdown() {
    if (!initialized_) {
        return false;
    }
    
    initialized_ = false;
    LOG_INFO("TimerDriver", "Timer driver shut down");
    return true;
}

ssize_t TimerDriver::read(void* buffer, size_t count) {
    if (!initialized_ || !buffer || count < sizeof(uint64_t)) {
        return -1;
    }
    
    *static_cast<uint64_t*>(buffer) = tickCount_;
    return sizeof(uint64_t);
}

ssize_t TimerDriver::write(const void* buffer, size_t count) {
    return -1;
}

bool TimerDriver::ioctl(uint32_t command, void* arg) {
    switch (command) {
        case 0:
            if (arg) {
                setFrequency(*static_cast<uint32_t*>(arg));
                return true;
            }
            break;
        case 1:
            tickCount_ = 0;
            startTime_ = std::chrono::steady_clock::now();
            return true;
    }
    return false;
}

void TimerDriver::tick() {
    if (initialized_) {
        tickCount_++;
    }
}

uint64_t TimerDriver::getElapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count();
}

void TimerDriver::setFrequency(uint32_t hz) {
    if (hz > 0 && hz <= 10000) {
        frequency_ = hz;
        LOG_INFO("TimerDriver", "Frequency set to " + std::to_string(hz) + " Hz");
    }
}

DriverManager::DriverManager() {
    LOG_INFO("DriverManager", "Initialized driver manager");
}

bool DriverManager::registerDriver(std::unique_ptr<Driver> driver) {
    if (!driver) {
        return false;
    }
    
    const std::string& name = driver->getName();
    if (drivers_.find(name) != drivers_.end()) {
        LOG_WARN("DriverManager", "Driver already registered: " + name);
        return false;
    }
    
    drivers_[name] = std::move(driver);
    LOG_INFO("DriverManager", "Registered driver: " + name);
    return true;
}

bool DriverManager::unregisterDriver(const std::string& name) {
    auto it = drivers_.find(name);
    if (it == drivers_.end()) {
        return false;
    }
    
    if (it->second->isInitialized()) {
        it->second->shutdown();
    }
    
    drivers_.erase(it);
    LOG_INFO("DriverManager", "Unregistered driver: " + name);
    return true;
}

Driver* DriverManager::getDriver(const std::string& name) {
    auto it = drivers_.find(name);
    return (it != drivers_.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> DriverManager::getDriverList() const {
    std::vector<std::string> list;
    for (const auto& [name, _] : drivers_) {
        list.push_back(name);
    }
    return list;
}

bool DriverManager::initAllDrivers() {
    bool success = true;
    for (auto& [name, driver] : drivers_) {
        if (!driver->isInitialized()) {
            if (!driver->init()) {
                LOG_ERROR("DriverManager", "Failed to initialize driver: " + name);
                success = false;
            }
        }
    }
    return success;
}

bool DriverManager::shutdownAllDrivers() {
    bool success = true;
    for (auto& [name, driver] : drivers_) {
        if (driver->isInitialized()) {
            if (!driver->shutdown()) {
                LOG_ERROR("DriverManager", "Failed to shut down driver: " + name);
                success = false;
            }
        }
    }
    return success;
}

std::string DriverManager::getDriverReport() const {
    std::stringstream ss;
    ss << "=== Driver Manager Report ===\n";
    ss << "Registered Drivers: " << drivers_.size() << "\n\n";
    
    ss << std::setw(15) << "Name" << " | "
       << std::setw(10) << "Type" << " | "
       << "Initialized" << "\n";
    ss << std::string(45, '-') << "\n";
    
    for (const auto& [name, driver] : drivers_) {
        std::string typeStr;
        switch (driver->getType()) {
            case DriverType::Character: typeStr = "Character"; break;
            case DriverType::Block: typeStr = "Block"; break;
            case DriverType::Network: typeStr = "Network"; break;
        }
        
        ss << std::setw(15) << name << " | "
           << std::setw(10) << typeStr << " | "
           << (driver->isInitialized() ? "Yes" : "No") << "\n";
    }
    
    return ss.str();
}

}
