#include "ipc/ipc.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace MiniOS;

void test_task_registration() {
    std::cout << "Testing task registration... ";
    
    IPCManager ipc;
    
    bool result = ipc.registerTask(1);
    assert(result == true);
    
    result = ipc.registerTask(1);
    assert(result == false);
    
    result = ipc.unregisterTask(1);
    assert(result == true);
    
    result = ipc.unregisterTask(1);
    assert(result == false);
    
    std::cout << "PASSED\n";
}

void test_message_sending() {
    std::cout << "Testing message sending... ";
    
    IPCManager ipc;
    ipc.registerTask(1);
    ipc.registerTask(2);
    
    int data = 42;
    MessageId id = ipc.sendMessage(1, 2, &data, sizeof(data));
    assert(id != 0);
    
    assert(ipc.hasMessages(2) == true);
    assert(ipc.getMessageCount(2) == 1);
    
    std::cout << "PASSED\n";
}

void test_message_receiving() {
    std::cout << "Testing message receiving... ";
    
    IPCManager ipc;
    ipc.registerTask(1);
    ipc.registerTask(2);
    
    struct TestMsg {
        int a;
        int b;
    };
    
    TestMsg sentMsg = {10, 20};
    ipc.sendMessage(1, 2, &sentMsg, sizeof(sentMsg));
    
    auto receivedMsg = ipc.receiveMessage(2, false);
    assert(receivedMsg.has_value());
    assert(receivedMsg->senderId == 1);
    
    auto data = receivedMsg->getData<TestMsg>();
    assert(data.has_value());
    assert(data->a == 10);
    assert(data->b == 20);
    
    assert(ipc.hasMessages(2) == false);
    
    std::cout << "PASSED\n";
}

void test_async_messaging() {
    std::cout << "Testing async messaging... ";
    
    IPCManager ipc;
    ipc.registerTask(1);
    ipc.registerTask(2);
    
    MessageId id1 = ipc.sendAsync(1, 2, nullptr, 0, MessageType::Notification);
    MessageId id2 = ipc.sendAsync(1, 2, nullptr, 0, MessageType::Signal);
    
    assert(id1 != 0);
    assert(id2 != 0);
    assert(id1 != id2);
    
    assert(ipc.getMessageCount(2) == 2);
    
    std::cout << "PASSED\n";
}

void test_message_types() {
    std::cout << "Testing message types... ";
    
    IPCManager ipc;
    ipc.registerTask(1);
    ipc.registerTask(2);
    
    ipc.sendMessage(1, 2, nullptr, 0, MessageType::Data);
    ipc.sendMessage(1, 2, nullptr, 0, MessageType::Signal);
    ipc.sendMessage(1, 2, nullptr, 0, MessageType::Request);
    
    auto msg1 = ipc.receiveMessage(2, false);
    assert(msg1.has_value() && msg1->type == MessageType::Data);
    
    auto msg2 = ipc.receiveMessage(2, false);
    assert(msg2.has_value() && msg2->type == MessageType::Signal);
    
    auto msg3 = ipc.receiveMessage(2, false);
    assert(msg3.has_value() && msg3->type == MessageType::Request);
    
    std::cout << "PASSED\n";
}

void test_no_messages() {
    std::cout << "Testing empty queue handling... ";
    
    IPCManager ipc;
    ipc.registerTask(1);
    
    assert(ipc.hasMessages(1) == false);
    assert(ipc.getMessageCount(1) == 0);
    
    auto msg = ipc.receiveMessage(1, false);
    assert(!msg.has_value());
    
    std::cout << "PASSED\n";
}

int main() {
    Logger::instance().setLevel(LogLevel::Error);
    
    std::cout << "\n=== IPC Unit Tests ===\n\n";
    
    test_task_registration();
    test_message_sending();
    test_message_receiving();
    test_async_messaging();
    test_message_types();
    test_no_messages();
    
    std::cout << "\nAll IPC tests passed!\n\n";
    return 0;
}
