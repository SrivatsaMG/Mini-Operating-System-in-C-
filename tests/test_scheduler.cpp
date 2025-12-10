#include "scheduler/scheduler.hpp"
#include <iostream>
#include <cassert>

using namespace MiniOS;

void test_task_creation() {
    std::cout << "Testing task creation... ";
    
    Scheduler scheduler(SchedulerType::RoundRobin);
    
    TaskId id1 = scheduler.createTask("test1", []() {}, TaskPriority::Normal);
    TaskId id2 = scheduler.createTask("test2", []() {}, TaskPriority::High);
    
    assert(id1 != INVALID_TASK_ID);
    assert(id2 != INVALID_TASK_ID);
    assert(id1 != id2);
    assert(scheduler.getTotalTasks() == 2);
    
    std::cout << "PASSED\n";
}

void test_task_states() {
    std::cout << "Testing task states... ";
    
    Scheduler scheduler(SchedulerType::RoundRobin);
    
    TaskId id = scheduler.createTask("test", []() {}, TaskPriority::Normal);
    
    TaskControlBlock* tcb = scheduler.getTask(id);
    assert(tcb != nullptr);
    assert(tcb->state == TaskState::Ready);
    
    scheduler.schedule();
    tcb = scheduler.getTask(id);
    assert(tcb->state == TaskState::Running);
    
    scheduler.blockTask(id);
    tcb = scheduler.getTask(id);
    assert(tcb->state == TaskState::Blocked);
    
    scheduler.unblockTask(id);
    tcb = scheduler.getTask(id);
    assert(tcb->state == TaskState::Ready);
    
    std::cout << "PASSED\n";
}

void test_round_robin() {
    std::cout << "Testing round-robin scheduling... ";
    
    Scheduler scheduler(SchedulerType::RoundRobin);
    
    TaskId id1 = scheduler.createTask("task1", []() {}, TaskPriority::Normal);
    TaskId id2 = scheduler.createTask("task2", []() {}, TaskPriority::Normal);
    TaskId id3 = scheduler.createTask("task3", []() {}, TaskPriority::Normal);
    
    scheduler.schedule();
    assert(scheduler.getCurrentTask()->id == id1);
    
    scheduler.yield();
    assert(scheduler.getCurrentTask()->id == id2);
    
    scheduler.yield();
    assert(scheduler.getCurrentTask()->id == id3);
    
    scheduler.yield();
    assert(scheduler.getCurrentTask()->id == id1);
    
    std::cout << "PASSED\n";
}

void test_priority_scheduling() {
    std::cout << "Testing priority scheduling... ";
    
    Scheduler scheduler(SchedulerType::Priority);
    
    scheduler.createTask("low", []() {}, TaskPriority::Low);
    TaskId idNormal = scheduler.createTask("normal", []() {}, TaskPriority::Normal);
    TaskId idHigh = scheduler.createTask("high", []() {}, TaskPriority::High);
    
    scheduler.schedule();
    assert(scheduler.getCurrentTask()->id == idHigh);
    
    scheduler.terminateTask(idHigh);
    
    TaskControlBlock* current = scheduler.getCurrentTask();
    assert(current != nullptr);
    assert(current->id == idNormal);
    
    std::cout << "PASSED\n";
}

void test_task_termination() {
    std::cout << "Testing task termination... ";
    
    Scheduler scheduler(SchedulerType::RoundRobin);
    
    TaskId id = scheduler.createTask("test", []() {}, TaskPriority::Normal);
    assert(scheduler.getTotalTasks() == 1);
    
    bool result = scheduler.terminateTask(id);
    assert(result == true);
    
    TaskControlBlock* tcb = scheduler.getTask(id);
    assert(tcb->state == TaskState::Terminated);
    
    std::cout << "PASSED\n";
}

int main() {
    Logger::instance().setLevel(LogLevel::Error);
    
    std::cout << "\n=== Scheduler Unit Tests ===\n\n";
    
    test_task_creation();
    test_task_states();
    test_round_robin();
    test_priority_scheduling();
    test_task_termination();
    
    std::cout << "\nAll scheduler tests passed!\n\n";
    return 0;
}
