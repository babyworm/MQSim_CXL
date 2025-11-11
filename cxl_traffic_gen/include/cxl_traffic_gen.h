#ifndef CXL_TRAFFIC_GEN_H
#define CXL_TRAFFIC_GEN_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace CXL {

/**
 * CXL Traffic Generator - Standalone CXL-Flash Memory Request Generator
 *
 * This class provides a simplified interface to the MQSim CXL-Flash simulator,
 * suitable for integration with Gem5 or standalone use.
 *
 * Key Features:
 * - Device-side DRAM cache with multiple replacement policies
 * - Hardware prefetchers (Tagged, Best-Offset, LEAP)
 * - MSHR for hit-under-miss support
 * - Full Flash backend simulation (FTL, GC, wear-leveling)
 * - Nanosecond-accurate discrete event simulation
 */
class TrafficGenerator {
public:
    //==========================================================================
    // Configuration
    //==========================================================================

    struct Config {
        // === CXL DRAM Cache Configuration ===
        uint64_t dram_size = 64 * 1024 * 1024;  // 64 MB default

        enum class CachePolicy {
            RANDOM,
            FIFO,
            LRU,
            LRU2,      // Two-level LRU
            LFU,
            LRFU,      // LRU + LFU
            CFLRU      // Clock-FIFO-LRU
        };
        CachePolicy cache_policy = CachePolicy::CFLRU;

        enum class PrefetcherType {
            NONE,
            TAGGED,        // Next-N-line
            BEST_OFFSET,   // Best-offset prefetcher
            LEAP,          // LEAP prefetcher
            FEEDBACK       // Feedback-directed
        };
        PrefetcherType prefetcher = PrefetcherType::BEST_OFFSET;

        bool has_mshr = true;                 // Enable MSHR
        uint16_t set_associativity = 16;      // N-way set-associative
        bool mix_mode = true;                 // Mix demand and prefetch data

        // === Flash Backend Configuration ===
        uint32_t num_channels = 8;
        uint32_t chips_per_channel = 8;
        uint32_t dies_per_chip = 1;
        uint32_t planes_per_die = 4;
        uint32_t blocks_per_plane = 512;
        uint32_t pages_per_block = 512;
        uint32_t page_size_bytes = 16384;     // 16 KB

        enum class FlashTechnology {
            SLC,  // Single-Level Cell
            MLC,  // Multi-Level Cell
            TLC   // Triple-Level Cell
        };
        FlashTechnology flash_tech = FlashTechnology::SLC;

        // Flash latencies (nanoseconds)
        uint64_t page_read_latency_ns = 3000;       // SLC: 3 us
        uint64_t page_program_latency_ns = 100000;  // SLC: 100 us
        uint64_t block_erase_latency_ns = 1000000;  // SLC: 1 ms

        // FTL Configuration
        double overprovisioning_ratio = 0.127;      // 12.7%
        double gc_threshold = 0.01;                 // Start GC at 1% free

        enum class GCPolicy {
            GREEDY,  // Greedy (most invalid pages)
            RGA,     // Randomized Greedy Algorithm
            RANDOM,
            FIFO
        };
        GCPolicy gc_policy = GCPolicy::GREEDY;

        // === DRAM Timing Parameters ===
        uint64_t dram_tRCD_ns = 13;  // RAS to CAS delay
        uint64_t dram_tCL_ns = 13;   // CAS latency
        uint64_t dram_tRP_ns = 13;   // Row precharge time

        // === Logging ===
        bool enable_logging = true;
        bool verbose = false;
    };

    //==========================================================================
    // Statistics
    //==========================================================================

    struct Statistics {
        // Cache statistics
        uint64_t total_requests = 0;
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        double hit_rate = 0.0;

        // Prefetch statistics
        uint64_t prefetch_issued = 0;
        uint64_t prefetch_hits = 0;
        uint64_t prefetch_pollution = 0;
        double prefetch_accuracy = 0.0;
        double prefetch_coverage = 0.0;

        // Flash backend statistics
        uint64_t flash_reads = 0;
        uint64_t flash_writes = 0;
        uint64_t flash_erases = 0;
        uint64_t gc_executions = 0;

        // Latency statistics (nanoseconds)
        uint64_t min_latency_ns = UINT64_MAX;
        uint64_t max_latency_ns = 0;
        uint64_t total_latency_ns = 0;
        double avg_latency_ns = 0.0;

        // Current simulation time
        uint64_t current_time_ns = 0;
    };

    //==========================================================================
    // Request Types
    //==========================================================================

    using RequestID = uint64_t;
    using Address = uint64_t;
    using CompletionCallback = std::function<void(RequestID, uint64_t latency_ns)>;

    //==========================================================================
    // Public API
    //==========================================================================

    /**
     * Constructor
     * @param config  Configuration parameters
     */
    explicit TrafficGenerator(const Config& config);

    /**
     * Destructor
     */
    ~TrafficGenerator();

    // Disable copy
    TrafficGenerator(const TrafficGenerator&) = delete;
    TrafficGenerator& operator=(const TrafficGenerator&) = delete;

    /**
     * Submit a read request
     * @param address   Physical address (byte-addressable)
     * @param size      Request size in bytes
     * @param callback  Completion callback (optional)
     * @return Request ID
     */
    RequestID submit_read(Address address, uint32_t size,
                          CompletionCallback callback = nullptr);

    /**
     * Submit a write request
     * @param address   Physical address
     * @param size      Request size in bytes
     * @param data      Data pointer (can be nullptr for simulation)
     * @param callback  Completion callback (optional)
     * @return Request ID
     */
    RequestID submit_write(Address address, uint32_t size,
                           const void* data = nullptr,
                           CompletionCallback callback = nullptr);

    /**
     * Advance simulation by one tick
     * @param time_ns  Current time in nanoseconds (absolute)
     */
    void tick(uint64_t time_ns);

    /**
     * Run simulation until specified time
     * @param target_time_ns  Target simulation time
     */
    void run_until(uint64_t target_time_ns);

    /**
     * Run until all pending requests complete
     * @param max_time_ns  Maximum time to wait (0 = unlimited)
     * @return true if all completed, false if timeout
     */
    bool run_until_complete(uint64_t max_time_ns = 0);

    /**
     * Check if there are pending requests
     */
    bool has_pending_requests() const;

    /**
     * Get current simulation time
     */
    uint64_t get_current_time_ns() const;

    /**
     * Get statistics
     */
    Statistics get_statistics() const;

    /**
     * Reset statistics (but not simulation state)
     */
    void reset_statistics();

    /**
     * Print statistics to stdout
     */
    void print_statistics() const;

private:
    class Impl;  // Forward declaration for PIMPL
    std::unique_ptr<Impl> pimpl_;
};

//==============================================================================
// Helper Functions
//==============================================================================

/**
 * Convert cache policy enum to string
 */
const char* to_string(TrafficGenerator::Config::CachePolicy policy);

/**
 * Convert prefetcher type enum to string
 */
const char* to_string(TrafficGenerator::Config::PrefetcherType type);

/**
 * Convert flash technology enum to string
 */
const char* to_string(TrafficGenerator::Config::FlashTechnology tech);

/**
 * Convert GC policy enum to string
 */
const char* to_string(TrafficGenerator::Config::GCPolicy policy);

} // namespace CXL

#endif // CXL_TRAFFIC_GEN_H
