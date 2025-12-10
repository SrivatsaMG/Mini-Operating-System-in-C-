#include "scheduler/scheduler.hpp"
#include <sstream>
#include <algorithm>

namespace MiniOS {

Scheduler::Scheduler(SchedulerType type)
    : type_(type)
    , nextTaskId_(1)
    , currentTaskId_(INVALID_TASK_ID)
    , tickCount_(0)
    , schedulerRunning_(false)
{
    LOG_INFO("Scheduler", "Initializing scheduler with " + 
             std::string(type == SchedulerType::RoundRobin ? "Round-Robin" : "Priority") + " algorithm");
}

TaskId Scheduler::createTask(const std::string& name, TaskFunction func, TaskPriority priority) {
    TaskId id = nextTaskId_++;
    
    auto tcb = std::make_unique<TaskControlBlock>(id, name, priority);
    tcb->state = TaskState::Ready;
    
    tasks_[id] = std::move(tcb);
    taskFunctions_[id] = std::move(func);
    
    addToReadyQueue(id);
    
    LOG_INFO("Scheduler", "Created task '" + name + "' with ID " + std::to_string(id) +
             " (Priority: " + tasks_[id]->priorityToString() + ")");
    
    return id;
}

bool Scheduler::terminateTask(TaskId id) {
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        LOG_ERROR("Scheduler", "Cannot terminate non-existent task " + std::to_string(id));
        return false;
    }
    
    it->second->state = TaskState::Terminated;
    removeFromReadyQueue(id);
    
    LOG_INFO("Scheduler", "Terminated task '" + it->second->name + "' (ID: " + std::to_string(id) + ")");
    
    if (currentTaskId_ == id) {
        currentTaskId_ = INVALID_TASK_ID;
        schedule();
    }
    
    return true;
}

bool Scheduler::blockTask(TaskId id) {
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return false;
    }
    
    if (it->second->state != TaskState::Running && it->second->state != TaskState::Ready) {
        return false;
    }
    
    it->second->state = TaskState::Blocked;
    removeFromReadyQueue(id);
    
    LOG_DEBUG("Scheduler", "Blocked task '" + it->second->name + "'");
    
    if (currentTaskId_ == id) {
        schedule();
    }
    
    return true;
}

bool Scheduler::unblockTask(TaskId id) {
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return false;
    }
    
    if (it->second->state != TaskState::Blocked) {
        return false;
    }
    
    it->second->state = TaskState::Ready;
    addToReadyQueue(id);
    
    LOG_DEBUG("Scheduler", "Unblocked task '" + it->second->name + "'");
    
    return true;
}

void Scheduler::schedule() {
    TaskId nextTask = selectNextTask();
    
    if (nextTask == INVALID_TASK_ID) {
        return;
    }
    
    if (nextTask == currentTaskId_) {
        return;
    }
    
    TaskControlBlock* currentTcb = getCurrentTask();
    TaskControlBlock* nextTcb = getTask(nextTask);
    
    if (currentTcb && currentTcb->state == TaskState::Running) {
        currentTcb->state = TaskState::Ready;
        addToReadyQueue(currentTaskId_);
    }
    
    contextSwitch(currentTcb, nextTcb);
    
    currentTaskId_ = nextTask;
    nextTcb->state = TaskState::Running;
    nextTcb->lastScheduledTime = std::chrono::steady_clock::now();
    nextTcb->timeSliceRemaining = TIME_QUANTUM_MS;
    
    removeFromReadyQueue(nextTask);
}

void Scheduler::tick() {
    tickCount_++;
    
    TaskControlBlock* current = getCurrentTask();
    if (!current) {
        schedule();
        return;
    }
    
    if (current->timeSliceRemaining > 0) {
        current->timeSliceRemaining--;
        current->cpuTimeMs++;
    }
    
    if (current->timeSliceRemaining == 0) {
        LOG_DEBUG("Scheduler", "Time slice expired for task '" + current->name + "'");
        schedule();
    }
}

void Scheduler::yield() {
    TaskControlBlock* current = getCurrentTask();
    if (current) {
        current->timeSliceRemaining = 0;
        LOG_DEBUG("Scheduler", "Task '" + current->name + "' yielded CPU");
        schedule();
    }
}

TaskControlBlock* Scheduler::getCurrentTask() {
    if (currentTaskId_ == INVALID_TASK_ID) {
        return nullptr;
    }
    auto it = tasks_.find(currentTaskId_);
    return (it != tasks_.end()) ? it->second.get() : nullptr;
}

TaskControlBlock* Scheduler::getTask(TaskId id) {
    auto it = tasks_.find(id);
    return (it != tasks_.end()) ? it->second.get() : nullptr;
}

void Scheduler::setSchedulerType(SchedulerType type) {
    if (type_ != type) {
        type_ = type;
        LOG_INFO("Scheduler", "Switched to " + 
                 std::string(type == SchedulerType::RoundRobin ? "Round-Robin" : "Priority") + " scheduling");
    }
}

size_t Scheduler::getReadyQueueSize() const {
    if (type_ == SchedulerType::RoundRobin) {
        return readyQueue_.size();
    } else {
        size_t total = 0;
        for (const auto& [_, queue] : priorityQueues_) {
            total += queue.size();
        }
        return total;
    }
}

void Scheduler::printTaskStates() const {
    std::cout << "\n=== Task States ===" << std::endl;
    std::cout << std::setw(6) << "ID" << " | "
              << std::setw(15) << "Name" << " | "
              << std::setw(10) << "State" << " | "
              << std::setw(8) << "Priority" << " | "
              << std::setw(8) << "CPU(ms)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (const auto& [id, tcb] : tasks_) {
        std::cout << std::setw(6) << id << " | "
                  << std::setw(15) << tcb->name << " | "
                  << std::setw(10) << tcb->stateToString() << " | "
                  << std::setw(8) << tcb->priorityToString() << " | "
                  << std::setw(8) << tcb->cpuTimeMs << std::endl;
    }
    std::cout << std::endl;
}

std::string Scheduler::getTaskReport() const {
    std::stringstream ss;
    ss << "=== Scheduler Report ===\n";
    ss << "Type: " << (type_ == SchedulerType::RoundRobin ? "Round-Robin" : "Priority") << "\n";
    ss << "Total Tasks: " << tasks_.size() << "\n";
    ss << "Ready Queue Size: " << getReadyQueueSize() << "\n";
    ss << "Current Task: " << (currentTaskId_ != INVALID_TASK_ID ? std::to_string(currentTaskId_) : "None") << "\n";
    ss << "Total Ticks: " << tickCount_ << "\n";
    return ss.str();
}

void Scheduler::contextSwitch(TaskControlBlock* from, TaskControlBlock* to) {
    if (from && to) {
        LOG_DEBUG("Scheduler", "Context switch: " + from->name + " -> " + to->name);
    } else if (to) {
        LOG_DEBUG("Scheduler", "Context switch: (none) -> " + to->name);
    }
}

TaskId Scheduler::selectNextTask() {
    if (type_ == SchedulerType::RoundRobin) {
        return selectRoundRobin();
    } else {
        return selectPriority();
    }
}

TaskId Scheduler::selectRoundRobin() {
    if (readyQueue_.empty()) {
        return INVALID_TASK_ID;
    }
    return readyQueue_.front();
}

TaskId Scheduler::selectPriority() {
    for (int p = static_cast<int>(TaskPriority::RealTime); p >= static_cast<int>(TaskPriority::Idle); --p) {
        auto priority = static_cast<TaskPriority>(p);
        auto it = priorityQueues_.find(priority);
        if (it != priorityQueues_.end() && !it->second.empty()) {
            return it->second.front();
        }
    }
    return INVALID_TASK_ID;
}

void Scheduler::addToReadyQueue(TaskId id) {
    auto it = tasks_.find(id);
    if (it == tasks_.end()) return;
    
    if (type_ == SchedulerType::RoundRobin) {
        if (std::find(readyQueue_.begin(), readyQueue_.end(), id) == readyQueue_.end()) {
            readyQueue_.push_back(id);
        }
    } else {
        auto& queue = priorityQueues_[it->second->priority];
        if (std::find(queue.begin(), queue.end(), id) == queue.end()) {
            queue.push_back(id);
        }
    }
}

void Scheduler::removeFromReadyQueue(TaskId id) {
    if (type_ == SchedulerType::RoundRobin) {
        auto it = std::find(readyQueue_.begin(), readyQueue_.end(), id);
        if (it != readyQueue_.end()) {
            readyQueue_.erase(it);
        }
    } else {
        for (auto& [_, queue] : priorityQueues_) {
            auto it = std::find(queue.begin(), queue.end(), id);
            if (it != queue.end()) {
                queue.erase(it);
                break;
            }
        }
    }
}

}
