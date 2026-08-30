// frame.hpp — the unit of work that flows down the pipeline.
//
// A Frame is metadata only: pointers into a slab slot, geometry, an index, and
// timestamps. The pixel bytes live immediately after the Frame
// header inside the same 64B-aligned slab slot, so one allocation yields both
// the header and its payload with a single cache-aligned block.
//
// Slot layout (one slab block):
//   [ Frame header | pad to 64 ] [ Y plane | U plane | V plane | pad to 64 ]
//    ^ slot base (64B aligned)    ^ data base (64B aligned) 

#pragma once 

#include <chrono> 
#include <cstdint> 
#include <new> 

#include "config.hpp" 

namespace vp { 

using Clock = std::chrono::steady_clock; 
using TimePoint = Clock::time_point;  

// Which physical memory layout the Y plane is stored in. Chroma is always
// raster(it's 8x8, so it's always cache friendly); only luma participates in the tiled cache experiment.
// More details here: https://gemini.google.com/app/2d2d9021914f7f39
enum class Layout {Raster, Tiled}; 

// round n up to the next multiple of a (a must be a power of two-ish multiple).
inline constexpr std::size_t align_up(std::size_t n, std::size_t a) { 
    return (n + a - 1)/a * a; 
} 

struct alignas(config::kCacheLine) Frame { 
    std::uint8_t* y = nullptr; // luma plane, kYSize bytes 
    std::uint8_t* u = nullptr; // Cb Plane, kCSize bytes 
    std::uint8_t* v = nullptr; // Cr plane, kCSize bytes 
    int width = 0; 
    int height = 0; 
    std::uint64_t index = 0; // monotonic (always increasing, ring buffer optimization) number from decode stage 
    Layout layout = Layout::Raster; 
    TimePoint arrival; // when the decode stage produced the frame 
    TimePoint deadline; // absolute "must be displayed by" deadline (WebRTC rules)
}; 

// Bytes reserved for the header portion of a slot (header rounded to a cache line).
inline constexpr std::size_t kFrameHeaderBytes = align_up(sizeof(Frame), config::kCacheLine); 

// Total size of one slab slot: aligned header + aligned pixel payload.
inline constexpr std::size_t frame_slot_size() { 
    return kFrameHeaderBytes + align_up(config::kFrameDataBytes, config::kCacheLine);
} 

// Construct a Frame at the start of a raw slab slot and wire its plane pointers
// to the payload region that follows the header. Returns the live Frame*. 
inline Frame* init_frame(void* slot, std::uint64_t index, Layout layout,  
                        TimePoint arrival, TimePoint deadline) {  
                            
        Frame* f = new (slot) Frame {}; 
        auto* base = reinterpret_cast<std::uint8_t*>(slot) + kFrameHeaderBytes; 
        f->y = base; 
        f->u = base + config::kYSize; 
        f->v = base + config::kYSize + config::kCSize; 
        f->width = config::kWidth; 
        f->height = config::kHeight; 
        f->index = index;  
        f->layout = layout;
        f->arrival = arrival; 
        f->deadline = deadline;
        return f;
    }

}