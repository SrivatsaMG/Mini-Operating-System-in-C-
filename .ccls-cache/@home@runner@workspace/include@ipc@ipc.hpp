#pragma once

#include "kernel/types.hpp"
#include "utils/logger.hpp"
#include <queue>
#include <map>
#include <vector>
#include <optional>
#include <mutex>
#include <condition_variable>

namespace MiniOS {

constexpr size_t MAX_MESSAGE_SIZE = 4096;

enum class MessageType {
    Data,
    Signal,
    Request,
    Response,
    Notification
};

struct Message {
    MessageId id;
    TaskId senderId;
    TaskId receiverId;
    MessageType type;
    std::vector<uint8_t> payload;
    std::chrono::steady_clock::time_point timestamp;
    bool isBlocking;

    Message(MessageId msgId, TaskId sender, TaskId receiver, MessageType t)
        : id(msgId)
        , senderId(sender)
        , receiverId(receiver)
        , type(t)
        , timestamp(std::chrono::steady_clock::now())
        , isBlocking(false)
    {}

    void setPayload(const void* data, size_t size) {
        if (size <= MAX_MESSAGE_SIZE) {
            payload.resize(size);
            std::memcpy(payload.data(), data, size);
        }
    }

    template<typename T>
    void setData(const T& data) {
        setPayload(&data, sizeof(T));
    }

    template<typename T>
    std::optional<T> getData() const {
        if (payload.size() >= sizeof(T)) {
            T data;
            std::memcpy(&data, payload.data(), sizeof(T));
            return data;
        }
        return std::nullopt;
    }
};

class MessageQueue {
public:
    explicit MessageQueue(TaskId owner);
    
    void enqueue(Message msg);
    std::optional<Message> dequeue();
    std::optional<Message> peek() const;
    
    bool isEmpty() const { return messages_.empty(); }
    size_t size() const { return messages_.size(); }
    TaskId getOwner() const { return owner_; }

private:
    TaskId owner_;
    std::queue<Message> messages_;
    mutable std::mutex mutex_;
};

class IPCManager {
public:
    IPCManager();
    ~IPCManager() = default;

    bool registerTask(TaskId taskId);
    bool unregisterTask(TaskId taskId);

    MessageId sendMessage(TaskId sender, TaskId receiver, 
                         const void* data, size_t size, 
                         MessageType type = MessageType::Data,
                         bool blocking = false);
    
    MessageId sendAsync(TaskId sender, TaskId receiver, 
                        const void* data, size_t size,
                        MessageType type = MessageType::Data);
    
    std::optional<Message> receiveMessage(TaskId receiver, bool blocking = true);
    std::optional<Message> receiveMessageFrom(TaskId receiver, TaskId sender, bool blocking = true);

    bool hasMessages(TaskId taskId) const;
    size_t getMessageCount(TaskId taskId) const;

    std::optional<Message> sendAndWaitReply(TaskId sender, TaskId receiver,
                                            const void* data, size_t size,
                                            std::chrono::milliseconds timeout);

    std::string getIPCReport() const;

private:
    std::map<TaskId, std::unique_ptr<MessageQueue>> messageQueues_;
    MessageId nextMessageId_;
    uint64_t totalMessagesSent_;
    uint64_t totalMessagesReceived_;
    mutable std::mutex mutex_;
};

}
