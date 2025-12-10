#pragma once

#include "kernel/types.hpp"
#include "utils/logger.hpp"
#include <map>
#include <vector>
#include <bitset>
#include <optional>

namespace MiniOS {

constexpr size_t TOTAL_PHYSICAL_FRAMES = 1024;
constexpr size_t VIRTUAL_ADDRESS_SPACE = 4096;

struct PageTableEntry {
    FrameNumber frameNumber;
    bool present;
    bool dirty;
    bool accessed;
    MemoryProtection protection;
    
    PageTableEntry() 
        : frameNumber(0), present(false), dirty(false), 
          accessed(false), protection(MemoryProtection::None) {}
};

struct PageTable {
    std::map<PageNumber, PageTableEntry> entries;
    TaskId ownerId;
    
    explicit PageTable(TaskId owner) : ownerId(owner) {}
};

class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager() = default;

    bool createAddressSpace(TaskId taskId);
    bool destroyAddressSpace(TaskId taskId);

    std::optional<void*> allocatePage(TaskId taskId, PageNumber virtualPage, 
                                       MemoryProtection protection = MemoryProtection::ReadWrite);
    bool freePage(TaskId taskId, PageNumber virtualPage);

    std::optional<FrameNumber> translateAddress(TaskId taskId, PageNumber virtualPage);
    bool handlePageFault(TaskId taskId, PageNumber virtualPage);

    bool setProtection(TaskId taskId, PageNumber virtualPage, MemoryProtection protection);
    std::optional<MemoryProtection> getProtection(TaskId taskId, PageNumber virtualPage);

    size_t getFreeFrameCount() const;
    size_t getUsedFrameCount() const;
    size_t getTaskMemoryUsage(TaskId taskId) const;

    std::string getMemoryReport() const;
    void printMemoryMap(TaskId taskId) const;

private:
    std::optional<FrameNumber> allocateFrame();
    bool freeFrame(FrameNumber frame);
    bool isFrameFree(FrameNumber frame) const;

    std::vector<uint8_t> physicalMemory_;
    std::bitset<TOTAL_PHYSICAL_FRAMES> frameAllocationMap_;
    std::map<TaskId, std::unique_ptr<PageTable>> pageTables_;
    
    size_t totalAllocatedPages_;
    size_t pageFaultCount_;
};

class HeapAllocator {
public:
    explicit HeapAllocator(size_t heapSize);
    ~HeapAllocator() = default;

    void* allocate(size_t size);
    void free(void* ptr);
    void* reallocate(void* ptr, size_t newSize);

    size_t getFreeMemory() const;
    size_t getUsedMemory() const;
    size_t getTotalMemory() const { return heapSize_; }

    std::string getHeapReport() const;

private:
    struct BlockHeader {
        size_t size;
        bool isFree;
        BlockHeader* next;
        BlockHeader* prev;
    };

    void coalesce(BlockHeader* block);
    BlockHeader* findFreeBlock(size_t size);
    void splitBlock(BlockHeader* block, size_t size);

    std::vector<uint8_t> heap_;
    size_t heapSize_;
    BlockHeader* freeList_;
    size_t allocatedBytes_;
};

}
