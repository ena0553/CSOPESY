#pragma once

#include <cstddef>
#include <string>

// Base interface for any memory allocation strategy (flat allocator, paging, etc.)
// MemoryManager implements this and selects PAGING as its strategy.
class IMemoryAllocator
{
public:
    enum MemoryAllocatorType
    {
        FLAT_MEMORY_ALLOCATOR,
        PAGING,
    };

    // Added a virtual destructor -- required any time a class is used
    // polymorphically through a base pointer (e.g. unique_ptr<IMemoryAllocator>),
    // otherwise a derived MemoryManager's destructor would never run.
    virtual ~IMemoryAllocator() = default;

    virtual void* allocate(size_t size) = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual std::string visualizeMemory() = 0; // "String" -> std::string

protected:
    MemoryAllocatorType memoryAllocatorType;

    // Describes a contiguous run of memory. Meant for allocators that hand out
    // one unbroken block per request (e.g. a flat/best-fit allocator).
    // See the discussion in the chat response for why a PAGING allocator does
    // NOT use this struct to track a process's memory.
    struct MemoryBlock
    {
        size_t start;
        size_t size;

        bool operator<(const MemoryBlock& other) const
        {
            return start < other.start;
        }
    };

    size_t maximumSize;
    size_t currentAllocatedSize;
};