/** 
 * Deterministic O(1) fixed-block pool
 * 
 * With this in place, we don't need to do a malloc/new for every frame
 * On construction, it grabs an arena, one CONTINOUS 64-B aligned arena  
 * and carves it into 'block_count' equal slots threaded into an  
 * intrusive-free Trieber list (intrusive because an idle slot's first 8 bytes 
 * hold the next-pointer - no separate metadata) 
 * 
 * allocate(), deallocate() are lock free via a CAS loop, so a producer thread (decode) 
 * and a consumer thread (render) never block each other
 */ 

 /** 
  * Concurrency model: Exactly one thread (decode) pops and only one thread pushes (render), actually stage 
  * two also pushes in some cases  
  * 
  * With just a single popper, the Trieber Stack is ABA free, so no pointer tagging/versioning is needed 
  * 
  * 
  */ 

#pragma once 

#include <atomic> 
#include <cstddef> 

namespace vp { 
    class SlabAllocator { 
        public:  
            // Block size is rounded up to alignment,  
            // each slot is atleast large enough to hold the intrusive-free list node
            SlabAllocator(std::size_t block_size, std::size_t block_count, 
                          std::size_t alignment = 64); 
            ~SlabAllocator(); 

            SlabAllocator(const SlabAllocator&) = delete; 
            SlabAllocator& operator=(const SlabAllocator&) = delete; 

            // Pop a free slot,or nullptr if the pool is exhausted. Never blocks
            void* allocate() noexcept; 

            // Return a slot to the pool. Never blocks (as it uses the Trieber stack instead of mutex)
            void* deallocate() noexcept; 

            std::size_t block_size() const noexcept {return block_size_;} 
            std::size_t capacity() const noexcept {return block_count_;} 

            // #blocks live but not allocated, must be 0 at shutdown
            std::size_t outstanding() const noexcept { 
                return outstanding_.load(std::memory_order_relaxed); // as this is for diagnostics
            } 

        private: 
            struct Node {Node* next;}; 

            std::size_t block_size_; 
            std::size_t block_count_; 
            std::size_t alignment_; 
            std::size_t arena_bytes_;     
            void* arena_; 

            std::atomic<Node*> free_head_; // Head of the intrusive free list
            std::atomic<std::size_t> outstanding_;
    };
}