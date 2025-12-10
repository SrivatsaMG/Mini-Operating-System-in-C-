#include "mm/memory_manager.hpp"
#include <sstream>
#include <iomanip>

namespace MiniOS {

MemoryManager::MemoryManager()
    : physicalMemory_(TOTAL_PHYSICAL_FRAMES * PAGE_SIZE, 0)
    , totalAllocatedPages_(0)
    , pageFaultCount_(0)
{
    frameAllocationMap_.reset();
    LOG_INFO("MemoryManager", "Initialized with " + std::to_string(TOTAL_PHYSICAL_FRAMES) + 
             " frames (" + std::to_string(TOTAL_PHYSICAL_FRAMES * PAGE_SIZE / 1024) + " KB)");
}

bool MemoryManager::createAddressSpace(TaskId taskId) {
    if (pageTables_.find(taskId) != pageTables_.end()) {
        LOG_WARN("MemoryManager", "Address space already exists for task " + std::to_string(taskId));
        return false;
    }
    
    pageTables_[taskId] = std::make_unique<PageTable>(taskId);
    LOG_INFO("MemoryManager", "Created address space for task " + std::to_string(taskId));
    return true;
}

bool MemoryManager::destroyAddressSpace(TaskId taskId) {
    auto it = pageTables_.find(taskId);
    if (it == pageTables_.end()) {
        return false;
    }
    
    for (auto& [pageNum, entry] : it->second->entries) {
        if (entry.present) {
            freeFrame(entry.frameNumber);
        }
    }
    
    pageTables_.erase(it);
    LOG_INFO("MemoryManager", "Destroyed address space for task " + std::to_string(taskId));
    return true;
}

std::optional<void*> MemoryManager::allocatePage(TaskId taskId, PageNumber virtualPage,
                                                  MemoryProtection protection) {
    auto ptIt = pageTables_.find(taskId);
    if (ptIt == pageTables_.end()) {
        LOG_ERROR("MemoryManager", "No address space for task " + std::to_string(taskId));
        return std::nullopt;
    }
    
    auto& pageTable = ptIt->second;
    if (pageTable->entries.find(virtualPage) != pageTable->entries.end() &&
        pageTable->entries[virtualPage].present) {
        LOG_WARN("MemoryManager", "Page " + std::to_string(virtualPage) + " already allocated");
        return std::nullopt;
    }
    
    auto frame = allocateFrame();
    if (!frame) {
        LOG_ERROR("MemoryManager", "Out of physical memory");
        return std::nullopt;
    }
    
    PageTableEntry entry;
    entry.frameNumber = *frame;
    entry.present = true;
    entry.dirty = false;
    entry.accessed = false;
    entry.protection = protection;
    
    pageTable->entries[virtualPage] = entry;
    totalAllocatedPages_++;
    
    void* physAddr = physicalMemory_.data() + (*frame * PAGE_SIZE);
    
    LOG_DEBUG("MemoryManager", "Allocated page " + std::to_string(virtualPage) + 
              " -> frame " + std::to_string(*frame) + " for task " + std::to_string(taskId));
    
    return physAddr;
}

bool MemoryManager::freePage(TaskId taskId, PageNumber virtualPage) {
    auto ptIt = pageTables_.find(taskId);
    if (ptIt == pageTables_.end()) {
        return false;
    }
    
    auto& pageTable = ptIt->second;
    auto entryIt = pageTable->entries.find(virtualPage);
    if (entryIt == pageTable->entries.end() || !entryIt->second.present) {
        return false;
    }
    
    freeFrame(entryIt->second.frameNumber);
    pageTable->entries.erase(entryIt);
    totalAllocatedPages_--;
    
    LOG_DEBUG("MemoryManager", "Freed page " + std::to_string(virtualPage) + 
              " for task " + std::to_string(taskId));
    
    return true;
}

std::optional<FrameNumber> MemoryManager::translateAddress(TaskId taskId, PageNumber virtualPage) {
    auto ptIt = pageTables_.find(taskId);
    if (ptIt == pageTables_.end()) {
        return std::nullopt;
    }
    
    auto& pageTable = ptIt->second;
    auto entryIt = pageTable->entries.find(virtualPage);
    if (entryIt == pageTable->entries.end() || !entryIt->second.present) {
        return std::nullopt;
    }
    
    entryIt->second.accessed = true;
    return entryIt->second.frameNumber;
}

bool MemoryManager::handlePageFault(TaskId taskId, PageNumber virtualPage) {
    pageFaultCount_++;
    LOG_DEBUG("MemoryManager", "Page fault for task " + std::to_string(taskId) + 
              " at page " + std::to_string(virtualPage));
    
    auto result = allocatePage(taskId, virtualPage);
    return result.has_value();
}

bool MemoryManager::setProtection(TaskId taskId, PageNumber virtualPage, MemoryProtection protection) {
    auto ptIt = pageTables_.find(taskId);
    if (ptIt == pageTables_.end()) {
        return false;
    }
    
    auto entryIt = ptIt->second->entries.find(virtualPage);
    if (entryIt == ptIt->second->entries.end()) {
        return false;
    }
    
    entryIt->second.protection = protection;
    return true;
}

std::optional<MemoryProtection> MemoryManager::getProtection(TaskId taskId, PageNumber virtualPage) {
    auto ptIt = pageTables_.find(taskId);
    if (ptIt == pageTables_.end()) {
        return std::nullopt;
    }
    
    auto entryIt = ptIt->second->entries.find(virtualPage);
    if (entryIt == ptIt->second->entries.end()) {
        return std::nullopt;
    }
    
    return entryIt->second.protection;
}

size_t MemoryManager::getFreeFrameCount() const {
    return TOTAL_PHYSICAL_FRAMES - frameAllocationMap_.count();
}

size_t MemoryManager::getUsedFrameCount() const {
    return frameAllocationMap_.count();
}

size_t MemoryManager::getTaskMemoryUsage(TaskId taskId) const {
    auto ptIt = pageTables_.find(taskId);
    if (ptIt == pageTables_.end()) {
        return 0;
    }
    
    size_t count = 0;
    for (const auto& [_, entry] : ptIt->second->entries) {
        if (entry.present) {
            count++;
        }
    }
    return count * PAGE_SIZE;
}

std::string MemoryManager::getMemoryReport() const {
    std::stringstream ss;
    ss << "=== Memory Manager Report ===\n";
    ss << "Total Physical Memory: " << (TOTAL_PHYSICAL_FRAMES * PAGE_SIZE / 1024) << " KB\n";
    ss << "Used Frames: " << getUsedFrameCount() << " / " << TOTAL_PHYSICAL_FRAMES << "\n";
    ss << "Free Frames: " << getFreeFrameCount() << "\n";
    ss << "Total Allocated Pages: " << totalAllocatedPages_ << "\n";
    ss << "Page Faults: " << pageFaultCount_ << "\n";
    ss << "Active Address Spaces: " << pageTables_.size() << "\n";
    return ss.str();
}

void MemoryManager::printMemoryMap(TaskId taskId) const {
    auto ptIt = pageTables_.find(taskId);
    if (ptIt == pageTables_.end()) {
        std::cout << "No address space for task " << taskId << std::endl;
        return;
    }
    
    std::cout << "\n=== Memory Map for Task " << taskId << " ===" << std::endl;
    std::cout << std::setw(10) << "VirtPage" << " | "
              << std::setw(10) << "Frame" << " | "
              << std::setw(8) << "Present" << " | "
              << std::setw(8) << "Dirty" << " | "
              << "Protection" << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    for (const auto& [page, entry] : ptIt->second->entries) {
        std::cout << std::setw(10) << page << " | "
                  << std::setw(10) << entry.frameNumber << " | "
                  << std::setw(8) << (entry.present ? "Yes" : "No") << " | "
                  << std::setw(8) << (entry.dirty ? "Yes" : "No") << " | "
                  << static_cast<int>(entry.protection) << std::endl;
    }
}

std::optional<FrameNumber> MemoryManager::allocateFrame() {
    for (FrameNumber i = 0; i < TOTAL_PHYSICAL_FRAMES; ++i) {
        if (!frameAllocationMap_[i]) {
            frameAllocationMap_.set(i);
            return i;
        }
    }
    return std::nullopt;
}

bool MemoryManager::freeFrame(FrameNumber frame) {
    if (frame >= TOTAL_PHYSICAL_FRAMES) {
        return false;
    }
    frameAllocationMap_.reset(frame);
    return true;
}

bool MemoryManager::isFrameFree(FrameNumber frame) const {
    if (frame >= TOTAL_PHYSICAL_FRAMES) {
        return false;
    }
    return !frameAllocationMap_[frame];
}

HeapAllocator::HeapAllocator(size_t heapSize)
    : heap_(heapSize)
    , heapSize_(heapSize)
    , allocatedBytes_(0)
{
    freeList_ = reinterpret_cast<BlockHeader*>(heap_.data());
    freeList_->size = heapSize - sizeof(BlockHeader);
    freeList_->isFree = true;
    freeList_->next = nullptr;
    freeList_->prev = nullptr;
    
    LOG_INFO("HeapAllocator", "Initialized heap with " + std::to_string(heapSize) + " bytes");
}

void* HeapAllocator::allocate(size_t size) {
    if (size == 0) return nullptr;
    
    size = (size + 7) & ~7;
    
    BlockHeader* block = findFreeBlock(size);
    if (!block) {
        LOG_ERROR("HeapAllocator", "Failed to allocate " + std::to_string(size) + " bytes");
        return nullptr;
    }
    
    if (block->size > size + sizeof(BlockHeader) + 8) {
        splitBlock(block, size);
    }
    
    block->isFree = false;
    allocatedBytes_ += block->size;
    
    return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(block) + sizeof(BlockHeader));
}

void HeapAllocator::free(void* ptr) {
    if (!ptr) return;
    
    BlockHeader* block = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader));
    
    if (block->isFree) {
        LOG_WARN("HeapAllocator", "Double free detected");
        return;
    }
    
    block->isFree = true;
    allocatedBytes_ -= block->size;
    
    coalesce(block);
}

void* HeapAllocator::reallocate(void* ptr, size_t newSize) {
    if (!ptr) return allocate(newSize);
    if (newSize == 0) {
        free(ptr);
        return nullptr;
    }
    
    BlockHeader* block = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader));
    
    if (block->size >= newSize) {
        return ptr;
    }
    
    void* newPtr = allocate(newSize);
    if (newPtr) {
        std::memcpy(newPtr, ptr, block->size);
        free(ptr);
    }
    return newPtr;
}

size_t HeapAllocator::getFreeMemory() const {
    return heapSize_ - allocatedBytes_ - sizeof(BlockHeader);
}

size_t HeapAllocator::getUsedMemory() const {
    return allocatedBytes_;
}

std::string HeapAllocator::getHeapReport() const {
    std::stringstream ss;
    ss << "=== Heap Allocator Report ===\n";
    ss << "Total Size: " << heapSize_ << " bytes\n";
    ss << "Used: " << allocatedBytes_ << " bytes\n";
    ss << "Free: " << getFreeMemory() << " bytes\n";
    ss << "Utilization: " << std::fixed << std::setprecision(1) 
       << (100.0 * allocatedBytes_ / heapSize_) << "%\n";
    return ss.str();
}

void HeapAllocator::coalesce(BlockHeader* block) {
    if (block->next && block->next->isFree) {
        block->size += sizeof(BlockHeader) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    if (block->prev && block->prev->isFree) {
        block->prev->size += sizeof(BlockHeader) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

HeapAllocator::BlockHeader* HeapAllocator::findFreeBlock(size_t size) {
    BlockHeader* current = freeList_;
    while (current) {
        if (current->isFree && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void HeapAllocator::splitBlock(BlockHeader* block, size_t size) {
    BlockHeader* newBlock = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<uint8_t*>(block) + sizeof(BlockHeader) + size);
    
    newBlock->size = block->size - size - sizeof(BlockHeader);
    newBlock->isFree = true;
    newBlock->next = block->next;
    newBlock->prev = block;
    
    if (block->next) {
        block->next->prev = newBlock;
    }
    
    block->size = size;
    block->next = newBlock;
}

}
