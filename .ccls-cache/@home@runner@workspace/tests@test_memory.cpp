#include "mm/memory_manager.hpp"
#include <iostream>
#include <cassert>

using namespace MiniOS;

void test_address_space() {
    std::cout << "Testing address space management... ";
    
    MemoryManager mm;
    
    bool result = mm.createAddressSpace(1);
    assert(result == true);
    
    result = mm.createAddressSpace(1);
    assert(result == false);
    
    result = mm.destroyAddressSpace(1);
    assert(result == true);
    
    result = mm.destroyAddressSpace(1);
    assert(result == false);
    
    std::cout << "PASSED\n";
}

void test_page_allocation() {
    std::cout << "Testing page allocation... ";
    
    MemoryManager mm;
    mm.createAddressSpace(1);
    
    size_t initialFree = mm.getFreeFrameCount();
    
    auto page = mm.allocatePage(1, 0);
    assert(page.has_value());
    assert(mm.getFreeFrameCount() == initialFree - 1);
    
    auto duplicate = mm.allocatePage(1, 0);
    assert(!duplicate.has_value());
    
    bool freed = mm.freePage(1, 0);
    assert(freed == true);
    assert(mm.getFreeFrameCount() == initialFree);
    
    mm.destroyAddressSpace(1);
    
    std::cout << "PASSED\n";
}

void test_address_translation() {
    std::cout << "Testing address translation... ";
    
    MemoryManager mm;
    mm.createAddressSpace(1);
    
    mm.allocatePage(1, 5, MemoryProtection::ReadWrite);
    
    auto frame = mm.translateAddress(1, 5);
    assert(frame.has_value());
    
    auto invalid = mm.translateAddress(1, 10);
    assert(!invalid.has_value());
    
    mm.destroyAddressSpace(1);
    
    std::cout << "PASSED\n";
}

void test_memory_protection() {
    std::cout << "Testing memory protection... ";
    
    MemoryManager mm;
    mm.createAddressSpace(1);
    
    mm.allocatePage(1, 0, MemoryProtection::Read);
    
    auto prot = mm.getProtection(1, 0);
    assert(prot.has_value());
    assert(*prot == MemoryProtection::Read);
    
    mm.setProtection(1, 0, MemoryProtection::ReadWrite);
    prot = mm.getProtection(1, 0);
    assert(*prot == MemoryProtection::ReadWrite);
    
    mm.destroyAddressSpace(1);
    
    std::cout << "PASSED\n";
}

void test_heap_allocator() {
    std::cout << "Testing heap allocator... ";
    
    HeapAllocator heap(1024 * 1024);
    
    void* ptr1 = heap.allocate(100);
    assert(ptr1 != nullptr);
    
    void* ptr2 = heap.allocate(200);
    assert(ptr2 != nullptr);
    
    size_t usedBefore = heap.getUsedMemory();
    
    heap.free(ptr1);
    assert(heap.getUsedMemory() < usedBefore);
    
    void* ptr3 = heap.allocate(50);
    assert(ptr3 != nullptr);
    
    heap.free(ptr2);
    heap.free(ptr3);
    
    std::cout << "PASSED\n";
}

void test_page_fault_handling() {
    std::cout << "Testing page fault handling... ";
    
    MemoryManager mm;
    mm.createAddressSpace(1);
    
    auto frame = mm.translateAddress(1, 100);
    assert(!frame.has_value());
    
    bool handled = mm.handlePageFault(1, 100);
    assert(handled == true);
    
    frame = mm.translateAddress(1, 100);
    assert(frame.has_value());
    
    mm.destroyAddressSpace(1);
    
    std::cout << "PASSED\n";
}

int main() {
    Logger::instance().setLevel(LogLevel::Error);
    
    std::cout << "\n=== Memory Manager Unit Tests ===\n\n";
    
    test_address_space();
    test_page_allocation();
    test_address_translation();
    test_memory_protection();
    test_heap_allocator();
    test_page_fault_handling();
    
    std::cout << "\nAll memory tests passed!\n\n";
    return 0;
}
