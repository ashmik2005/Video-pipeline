/** 
*
    Everything here is a plain constant so the numbers show up in one place and
    the story ("16-frame arena, 4K YUV420, 16x16 tiles") is grounded in
    code. No dependency on any other project header, so it can
    be included from anywhere without creating cycles.
 * 
 * 
 */ 

#pragma once 

#include <cstddef> 
#include <cstdint> 

namespace vp::config{ 

// ---- Frame geometry: 4K (2160p) YUV 4:2:0 planar --------------------------
    inline constexpr int kWidth  = 3840;
    inline constexpr int kHeight = 2160; 

// YUV420: full-res luma (Y), quarter-res chroma (U, V) each.
    inline constexpr std::size_t kYSize = static_cast<std::size_t>(kWidth) * kHeight; 
    inline constexpr std::size_t kCSize = static_cast<std::size_t>(kWidth/2)*(kHeight/2); 
    inline constexpr std::size_t kFrameDataBytes = kYSize + 2*kCSize; 

// Cache layout 
    inline constexpr std::size_t kCacheLine = 64; // x86-64 cache line size, reason for alignas(64) choices 
    inline constexpr int         kTile = 16;      // Classic 16x16 Video macroblock size 

// Memory subsystem parameters 
    inline constexpr std::size_t kArenaFrames = 16; // Slab holds 16 frame slots at a time (~190 MiB) 
    inline constexpr std::size_t kRingCapacity = 8; // SPSC Ring capacity, should be < arena size (sanity) and a power of two for efficient bitwise math  

// For enforcing RTC Admission control policies (same as WebRTC) 
    inline constexpr int        kDefaultFps = 60; 
    inline constexpr std::uint64_t kDefaultFrames = 600; // ~10s of 60 fps content by default 
    
    static_assert(kRingCapacity < kArenaFrames, "ring must not be able to hold every slot at once");
    static_assert((kRingCapacity & (kRingCapacity - 1)) == 0, "ring capacity must be a power of two");
    static_assert(kWidth % kTile == 0 && kHeight % kTile == 0, "frame must tile evenly into 16x16 blocks"); 

}   // namespace vp::config