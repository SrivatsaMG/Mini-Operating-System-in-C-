#include "ipc/ipc.hpp"
#include <sstream>

namespace MiniOS {

MessageQueue::MessageQueue(TaskId owner)
    : owner_(owner)
{
}

void MessageQueue::enqueue(Message msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push(std::move(msg));
}

std::optional<Message> MessageQueue::dequeue() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (messages_.empty()) {
        return std::nullopt;
    }
    
    Message msg = std::move(messages_.front());
    messages_.pop();
    return msg;
}

std::optional<Message> MessageQueue::peek() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (messages_.empty()) {
        return std::nullopt;
    }
    return messages_.front();
}

IPCManager::IPCManager()
    : nextMessageId_(1)
    , totalMessagesSent_(0)
    , totalMessagesReceived_(0)
{
    LOG_INFO("IPC", "Initialized IPC Manager");
}

bool IPCManager::registerTask(TaskId taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (messageQueues_.find(taskId) != messageQueues_.end()) {
        LOG_WARN("IPC", "Task " + std::to_string(taskId) + " already registered");
        return false;
    }
    
    messageQueues_[taskId] = std::make_unique<MessageQueue>(taskId);
    LOG_DEBUG("IPC", "Registered task " + std::to_string(taskId) + " for IPC");
    return true;
}

bool IPCManager::unregisterTask(TaskId taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = messageQueues_.find(taskId);
    if (it == messageQueues_.end()) {
        return false;
    }
    
    messageQueues_.erase(it);
    LOG_DEBUG("IPC", "Unregistered task " + std::to_string(taskId) + " from IPC");
    return true;
}

MessageId IPCManager::sendMessage(TaskId sender, TaskId receiver,
                                   const void* data, size_t size,
                                   MessageType type, bool blocking) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = messageQueues_.find(receiver);
    if (it == messageQueues_.end()) {
        LOG_ERROR("IPC", "Cannot send to unregistered task " + std::to_string(receiver));
        return 0;
    }
    
    MessageId id = nextMessageId_++;
    Message msg(id, sender, receiver, type);
    msg.isBlocking = blocking;
    
    if (data && size > 0) {
        msg.setPayload(data, size);
    }
    
    it->second->enqueue(std::move(msg));
    totalMessagesSent_++;
    
    LOG_DEBUG("IPC", "Message " + std::to_string(id) + " sent from " + 
              std::to_string(sender) + " to " + std::to_string(receiver));
    
    return id;
}

MessageId IPCManager::sendAsync(TaskId sender, TaskId receiver,
                                 const void* data, size_t size, MessageType type) {
    return sendMessage(sender, receiver, data, size, type, false);
}

std::optional<Message> IPCManager::receiveMessage(TaskId receiver, bool blocking) {
    auto it = messageQueues_.find(receiver);
    if (it == messageQueues_.end()) {
        return std::nullopt;
    }
    
    auto msg = it->second->dequeue();
    if (msg) {
        totalMessagesReceived_++;
        LOG_DEBUG("IPC", "Message " + std::to_string(msg->id) + " received by " + 
                  std::to_string(receiver));
    }
    
    return msg;
}

std::optional<Message> IPCManager::receiveMessageFrom(TaskId receiver, TaskId sender, bool blocking) {
    auto it = messageQueues_.find(receiver);
    if (it == messageQueues_.end()) {
        return std::nullopt;
    }
    
    auto msg = it->second->peek();
    if (msg && msg->senderId == sender) {
        return it->second->dequeue();
    }
    
    return std::nullopt;
}

bool IPCManager::hasMessages(TaskId taskId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = messageQueues_.find(taskId);
    if (it == messageQueues_.end()) {
        return false;
    }
    return !it->second->isEmpty();
}

size_t IPCManager::getMessageCount(TaskId taskId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = messageQueues_.find(taskId);
    if (it == messageQueues_.end()) {
        return 0;
    }
    return it->second->size();
}

std::optional<Message> IPCManager::sendAndWaitReply(TaskId sender, TaskId receiver,
                                                     const void* data, size_t size,
                                                     std::chrono::milliseconds timeout) {
    MessageId msgId = sendMessage(sender, receiver, data, size, MessageType::Request, true);
    if (msgId == 0) {
        return std::nullopt;
    }
    
    auto startTime = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - startTime < timeout) {
        auto reply = receiveMessageFrom(sender, receiver, false);
        if (reply && reply->type == MessageType::Response) {
            return reply;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOG_WARN("IPC", "Timeout waiting for reply from " + std::to_string(receiver));
    return std::nullopt;
}

std::string IPCManager::getIPCReport() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    ss << "=== IPC Manager Report ===\n";
    ss << "Registered Tasks: " << messageQueues_.size() << "\n";
    ss << "Total Messages Sent: " << totalMessagesSent_ << "\n";
    ss << "Total Messages Received: " << totalMessagesReceived_ << "\n";
    ss << "Next Message ID: " << nextMessageId_ << "\n";
    
    ss << "\nPending Messages per Task:\n";
    for (const auto& [taskId, queue] : messageQueues_) {
        ss << "  Task " << taskId << ": " << queue->size() << " messages\n";
    }
    
    return ss.str();
}

}
