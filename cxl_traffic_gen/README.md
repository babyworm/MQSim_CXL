# CXL Traffic Generator

> Standalone CXL-Flash Memory Traffic Generator extracted from MQSim_CXL

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## 📌 Overview

**CXL Traffic Generator** is a lightweight, standalone library extracted from [MQSim_CXL](https://github.com/dgist-datalab/MQSim_CXL) that simulates CXL-enabled flash memory devices. It's designed for:

- ✅ **Gem5 Integration**: Use as a CXL traffic generator in Gem5 simulations
- ✅ **Standalone Simulation**: Run memory access simulations independently
- ✅ **Research**: Study CXL cache behavior, prefetching, and flash backend performance

### Key Features

- **Device-side DRAM Cache**: 64MB-8GB configurable cache with multiple replacement policies
- **Hardware Prefetchers**: Tagged (next-N-line), Best-Offset, LEAP, Feedback-directed
- **MSHR Support**: Hit-under-miss capability for overlapping misses
- **Full Flash Backend**: FTL with address mapping, garbage collection, wear-leveling
- **Nanosecond Accuracy**: Discrete event simulation with ns-level timing
- **Simple API**: Clean C++ interface for easy integration

---

## 🚀 Quick Start

### Standalone Usage

```cpp
#include "cxl_traffic_gen.h"

int main() {
    // Configure
    CXL::TrafficGenerator::Config config;
    config.dram_size = 64 * 1024 * 1024;  // 64 MB
    config.cache_policy = CXL::TrafficGenerator::Config::CachePolicy::CFLRU;
    config.prefetcher = CXL::TrafficGenerator::Config::PrefetcherType::BEST_OFFSET;

    // Create generator
    CXL::TrafficGenerator gen(config);

    // Submit requests
    gen.submit_read(0x1000, 4096, [](uint64_t id, uint64_t latency_ns) {
        std::cout << "Request " << id << " completed in " << latency_ns << " ns\n";
    });

    // Run simulation
    gen.run_until_complete();

    // Print statistics
    gen.print_statistics();
}
```

### Gem5 Integration

```cpp
// In Gem5 SimObject
class CXLFlashDevice : public SimObject {
    std::unique_ptr<CXL::TrafficGenerator> traffic_gen_;

    bool recvTimingReq(PacketPtr pkt) override {
        traffic_gen_->submit_read(pkt->getAddr(), pkt->getSize(),
            [this, pkt](uint64_t id, uint64_t latency_ns) {
                Tick ticks = SimClock::Int::ns * latency_ns;
                mem_side_port.schedTimingResp(pkt, curTick() + ticks);
            });
        return true;
    }

    void tick() {
        traffic_gen_->tick(curTick() / SimClock::Int::ns);
    }
};
```

---

## 📦 Building

### Prerequisites

- **C++14** or later
- **CMake 3.10+**
- **Make** or **Ninja**

### Build Steps

```bash
cd cxl_traffic_gen
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run Examples

```bash
# Standalone example
./standalone_example

# Gem5 integration example (pseudo-code)
./gem5_integration_example
```

---

## 📚 API Reference

### Configuration

```cpp
CXL::TrafficGenerator::Config config;

// === CXL DRAM Cache ===
config.dram_size = 64 * 1024 * 1024;         // 64 MB
config.cache_policy = CachePolicy::CFLRU;     // FIFO, LRU, CFLRU, etc.
config.prefetcher = PrefetcherType::BEST_OFFSET;  // NONE, TAGGED, BEST_OFFSET, LEAP
config.has_mshr = true;                       // Enable hit-under-miss
config.set_associativity = 16;                // 16-way set-associative

// === Flash Backend ===
config.num_channels = 8;
config.chips_per_channel = 8;
config.flash_tech = FlashTechnology::SLC;     // SLC, MLC, TLC
config.page_read_latency_ns = 3000;           // 3 μs
config.page_program_latency_ns = 100000;      // 100 μs
config.block_erase_latency_ns = 1000000;      // 1 ms

// === FTL Configuration ===
config.overprovisioning_ratio = 0.127;        // 12.7%
config.gc_threshold = 0.01;                   // Start GC at 1% free
config.gc_policy = GCPolicy::GREEDY;          // GREEDY, RGA, RANDOM, FIFO

// === DRAM Timing ===
config.dram_tRCD_ns = 13;  // RAS to CAS delay
config.dram_tCL_ns = 13;   // CAS latency
config.dram_tRP_ns = 13;   // Row precharge
```

### Submitting Requests

```cpp
// Read request
auto req_id = gen.submit_read(
    0x1000,      // Address (byte-addressable)
    4096,        // Size in bytes
    [](uint64_t id, uint64_t latency_ns) {  // Callback (optional)
        // Handle completion
    }
);

// Write request
auto req_id = gen.submit_write(
    0x2000,      // Address
    4096,        // Size
    nullptr,     // Data pointer (can be nullptr for simulation)
    [](uint64_t id, uint64_t latency_ns) {
        // Handle completion
    }
);
```

### Running Simulation

```cpp
// Advance by one time step
gen.tick(current_time_ns);

// Run until specific time
gen.run_until(1'000'000);  // Run until 1 ms

// Run until all requests complete
bool completed = gen.run_until_complete(10'000'000'000);  // 10s timeout

// Check if there are pending requests
bool pending = gen.has_pending_requests();
```

### Statistics

```cpp
auto stats = gen.get_statistics();

std::cout << "Cache Hit Rate: " << (stats.hit_rate * 100.0) << "%\n";
std::cout << "Avg Latency: " << stats.avg_latency_ns << " ns\n";
std::cout << "Flash Reads: " << stats.flash_reads << "\n";
std::cout << "Prefetch Accuracy: " << (stats.prefetch_accuracy * 100.0) << "%\n";

// Or print all statistics
gen.print_statistics();

// Reset statistics (not simulation state)
gen.reset_statistics();
```

---

## 🏗️ Architecture

### Component Diagram

```
┌─────────────────────────────────────────┐
│     CXL Traffic Generator (Public API)  │
└──────────────┬──────────────────────────┘
               │
        ┌──────▼──────┐
        │ CXL_Manager │ ← Cache hit/miss decision
        └──────┬──────┘
               │
       ┌───────┴────────┐
       │                │
┌──────▼─────┐   ┌─────▼──────┐
│DRAM Cache  │   │   MSHR     │
│Policies:   │   │Hit-under-  │
│FIFO/LRU/   │   │miss support│
│CFLRU       │   └────────────┘
└──────┬─────┘
       │ Cache Miss
       │
┌──────▼──────────────┐
│  Flash Backend      │
│  ┌───────────────┐  │
│  │ FTL           │  │
│  │ - Address Map │  │
│  │ - GC          │  │
│  │ - Wear Level  │  │
│  └───────┬───────┘  │
│          │          │
│  ┌───────▼───────┐  │
│  │ Flash Chips   │  │
│  │ 8ch × 8chips  │  │
│  └───────────────┘  │
└─────────────────────┘
```

### Data Flow

```
1. submit_read(addr) → CXL_Manager
2. CXL_Manager → DRAM_Subsystem::isCacheHit()
3a. [HIT]  → Return cached data (100ns)
3b. [MISS] → Check MSHR
4a. [HIT-UNDER-MISS] → Wait for ongoing request
4b. [NEW MISS] → Forward to Flash Backend
5. Flash read (3-75 μs depending on SLC/MLC/TLC)
6. Fill DRAM cache (evict if needed)
7. Callback with latency
```

---

## 📊 Performance Characteristics

| Scenario | Latency | Description |
|----------|---------|-------------|
| **Cache Hit** | ~100 ns | DRAM access (tRCD + tCL + transfer) |
| **Cache Miss (SLC)** | ~3 μs | Flash read latency |
| **Cache Miss (MLC)** | ~25 μs | MLC read latency |
| **Cache Miss (TLC)** | ~75 μs | TLC MSB read latency |
| **Hit-under-Miss** | Same as miss | No duplicate flash access |
| **Prefetch Hit** | ~100 ns | Data already prefetched |

### Example Statistics

**Sequential Access (100 requests, 4KB each)**:
```
Cache Hit Rate: 92.0%
Prefetch Accuracy: 85.0%
Avg Latency: 320 ns
Flash Reads: 8
```

**Random Access (100 requests, 4KB each)**:
```
Cache Hit Rate: 15.0%
Prefetch Accuracy: 12.0%
Avg Latency: 2550 ns
Flash Reads: 85
```

---

## 🔧 Advanced Configuration

### Cache Replacement Policies

| Policy | Description | Complexity | Best For |
|--------|-------------|------------|----------|
| **RANDOM** | Random eviction | O(1) | Baseline |
| **FIFO** | First-in-first-out | O(1) | Simple workloads |
| **LRU** | Least recently used | O(1) | General purpose |
| **LRU-2** | Two-level LRU | O(1) | Scan-resistant |
| **LFU** | Least frequently used | O(log N) | Frequency-based |
| **LRFU** | LRU + LFU hybrid | O(log N) | Mixed patterns |
| **CFLRU** | Clock + FIFO + LRU | O(1) | High performance |

### Prefetcher Algorithms

| Prefetcher | Description | Best For |
|------------|-------------|----------|
| **NONE** | No prefetching | Random access |
| **TAGGED** | Next-N-line | Sequential access |
| **BEST_OFFSET** | Learns optimal offset | Strided access |
| **LEAP** | Signature-based | Irregular patterns |
| **FEEDBACK** | Adaptive based on accuracy | Mixed workloads |

---

## 🔬 Research Use Cases

1. **CXL Cache Evaluation**: Study different cache sizes and policies
2. **Prefetcher Design**: Compare prefetching algorithms
3. **Flash Backend Analysis**: Evaluate GC policies and wear-leveling
4. **Gem5 Integration**: Accurate CXL device modeling in full-system simulation
5. **Memory Expansion**: Study CXL as main memory extension

---

## 📖 Examples

See `examples/` directory:
- `standalone_example.cpp` - Basic standalone usage
- `gem5_integration_example.cpp` - Gem5 SimObject integration (pseudo-code)

---

## 🤝 Contributing

This is an extracted and simplified version of MQSim_CXL. For the full simulator, see:
- **Original MQSim_CXL**: https://github.com/dgist-datalab/MQSim_CXL
- **Paper**: "Overcoming the Memory Wall with CXL-Enabled SSDs" (USENIX ATC'23)

---

## 📄 License

MIT License (same as MQSim_CXL)

---

## 🙏 Acknowledgments

Based on [MQSim_CXL](https://github.com/dgist-datalab/MQSim_CXL) by:
- S4 Group (Syracuse University)
- DGIST DataLab
- DATOS Lab (Soongsil University)
- FADU

---

## 📞 Contact

For questions or issues:
- Open an issue on GitHub
- Refer to the original MQSim_CXL repository

---

**Note**: This is a **simplified extraction** for traffic generation purposes. For full research-grade simulation with trace support, XML configuration, and comprehensive FTL features, use the complete [MQSim_CXL](https://github.com/dgist-datalab/MQSim_CXL).
