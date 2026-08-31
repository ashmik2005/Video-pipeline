#include "slab_allocator.hpp" 

#include <cassert> 
#include <cstdlib> 
#include <new> 

namespace vp { 
    SlabAllocator::SlabAllocator(std::size_t block_size, std::size_t block_count, 
                                std::size_t alignment) : block_count_(block_count), alignment_(alignment) { 
        
        // A slot must hold at least the free-list node, and stay aligned so every  
        // slot base sits on a cache line boundary
        if(block_size < sizeof(Node)) block_size = sizeof(Node); 

        block_size_ = (block_size + alignment - 1) / alignment * alignment; 
        arena_bytes_ = block_size_*block_count_; 
        
        // Memory for the entire pool using just one syscall, that too aligned
        if(::posix_memalign(&arena_, alignment_, arena_bytes_) != 0) { 
            throw std::bad_alloc();
        } 

        // Now, we have the arena, we need to chop it up into slots, 
        // We will put it in the stack so that slot 0 is at the top (easier debugging) 
        Node* head = nullptr; 
        auto* base = reinterpret_cast<unsigned char*>(arena_); 

        for(std::size_t i = block_count_; i-- > 0;) { 
            auto* node = reinterpret_cast<Node*>(base + i*block_size_); 
            node->next = head; 
            head = node;
        } 

        free_head_.store(head, std::memory_order_relaxed); 
        outstanding_.store(0, std::memory_order_relaxed);


    }
}