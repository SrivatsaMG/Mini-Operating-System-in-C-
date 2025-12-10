#pragma once

#include "kernel/types.hpp"
#include "scheduler/tcb.hpp"
#include "utils/logger.hpp"
#include <queue>
#include <map>
#include <functional>
#include <memory>

namespace MiniOS {

enum class SchedulerType {
    RoundRobin,
    Priority
};

class Scheduler {
public:
    using TaskFunction = std::function<void()>;

    explicit Scheduler(SchedulerType type = SchedulerType::RoundRobin);
    ~Scheduler() = default;

    TaskId createTask(const std::string& name, TaskFunction func, TaskPriority priority = TaskPriority::Normal);
    bool terminateTask(TaskId id);
    bool blockTask(TaskId id);
    bool unblockTask(TaskId id);
    
    void schedule();
    void tick();
    void yield();
    
    TaskControlBlock* getCurrentTask();
    TaskControlBlock* getTask(TaskId id);
    
    void setSchedulerType(SchedulerType type);
    SchedulerType getSchedulerType() const { return type_; }
    
    size_t getReadyQueueSize() const;
    size_t getTotalTasks() const { return tasks_.size(); }
    
    void printTaskStates() const;
    std::string getTaskReport() const;

private:
    void contextSwitch(TaskControlBlock* from, TaskControlBlock* to);
    TaskId selectNextTask();
    TaskId selectRoundRobin();
    TaskId selectPriority();
    void addToReadyQueue(TaskId id);
    void removeFromReadyQueue(TaskId id);

    SchedulerType type_;
    TaskId nextTaskId_;
    TaskId currentTaskId_;
    
    std::map<TaskId, std::unique_ptr<TaskControlBlock>> tasks_;
    std::map<TaskId, TaskFunction> taskFunctions_;
    std::deque<TaskId> readyQueue_;
    std::map<TaskPriority, std::deque<TaskId>> priorityQueues_;
    
    uint64_t tickCount_;
    bool schedulerRunning_;
};

}
