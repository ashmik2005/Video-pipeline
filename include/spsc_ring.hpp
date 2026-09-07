/** 
 * Lock-free SPSC ring
 * 
 */

#pragma once 

#include <array> 
#include <atomic>
#include <cstddef> 

namespace vp { 
    template<typename T, std::size_t N> 
    class SpscRing { 
        static_assert(N >= 2, "Ring needs at least two slots") 
        static_assert((N & (N - 1)) == 0, "N must be a power of 2") 

        private:  
        // Each index on it's own cache line, no false sharing between producer and consumer
            alignas(64) std::atomic<std::size_t> write_index_{0}; 
            alignas(64) std::atomic<std::size_t> read_index_{0}; 
            alignas(64) std::array<T, N> buffer_{}; 

        public:  

            // Producer. Returns false if ring is full
            bool try_push(const T& value) noexcept {     
                const std::size_t w = write_index_.load(std::memory_order_relaxed); // producer owns this 
                const std::size_t r = read_index_.load(std::memory_order_acquire); // sync with consumer 

                if(w - r == N) return false;  // buffer is full

                buffer_[w & (N-1)] = value;  
                write_index_.store(w + 1, std::memory_order_release); // publish the write
                return true;
            } 

            bool try_pop(T& out) noexcept { 
                const std::size_t r = read_index_.load(std::memory_order_relaxed); 
                const std::size_t w = write_index_.load(std::memory_order_acquire); // sync with producer 

                if(w == r) return false; // empty

                out = buffer[r & (N - 1)]; 
                read_index_.store(r + 1, std::memory_order_release); // publish read, free slot
                return true;

            } 

            bool empty() const noexcept { 
                return write_index.load(std::memory_order_acquire)  ==
                         read_index_.load(std::memory_order_acquire); 
            } 

            static constexpr std::size_t capacity() noexcept {return N;}

    
    };
}