/**
 * Standalone Example - CXL Traffic Generator
 *
 * This example demonstrates basic usage of the CXL Traffic Generator
 * without any external dependencies (e.g., Gem5).
 */

#include "cxl_traffic_gen.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace CXL;

int main() {
    std::cout << "=== CXL Traffic Generator - Standalone Example ===\n\n";

    // =========================================================================
    // 1. Configure the Traffic Generator
    // =========================================================================

    TrafficGenerator::Config config;

    // CXL DRAM Cache
    config.dram_size = 64 * 1024 * 1024;  // 64 MB
    config.cache_policy = TrafficGenerator::Config::CachePolicy::CFLRU;
    config.prefetcher = TrafficGenerator::Config::PrefetcherType::BEST_OFFSET;
    config.has_mshr = true;
    config.set_associativity = 16;

    // Flash Backend
    config.num_channels = 8;
    config.chips_per_channel = 8;
    config.flash_tech = TrafficGenerator::Config::FlashTechnology::SLC;
    config.page_read_latency_ns = 3000;      // 3 μs
    config.page_program_latency_ns = 100000; // 100 μs

    // Logging
    config.enable_logging = true;
    config.verbose = false;

    std::cout << "Configuration:\n";
    std::cout << "  DRAM Size: " << (config.dram_size / 1024 / 1024) << " MB\n";
    std::cout << "  Cache Policy: " << to_string(config.cache_policy) << "\n";
    std::cout << "  Prefetcher: " << to_string(config.prefetcher) << "\n";
    std::cout << "  Flash Channels: " << config.num_channels << "\n";
    std::cout << "  Chips/Channel: " << config.chips_per_channel << "\n";
    std::cout << "\n";

    // =========================================================================
    // 2. Create Traffic Generator
    // =========================================================================

    TrafficGenerator gen(config);

    // =========================================================================
    // 3. Submit Read Requests
    // =========================================================================

    std::cout << "Submitting read requests...\n";

    std::vector<TrafficGenerator::RequestID> request_ids;
    uint64_t completed_count = 0;

    // Sequential reads (should benefit from prefetching)
    for (int i = 0; i < 100; i++) {
        uint64_t address = i * 4096;  // 4 KB aligned

        auto req_id = gen.submit_read(address, 4096,
            [&completed_count, address](TrafficGenerator::RequestID id, uint64_t latency) {
                completed_count++;
                if (completed_count % 10 == 0) {
                    std::cout << "  Request " << id << " (addr=0x" << std::hex << address << std::dec
                              << ") completed in " << latency << " ns\n";
                }
            });

        request_ids.push_back(req_id);
    }

    std::cout << "\n";

    // =========================================================================
    // 4. Run Simulation
    // =========================================================================

    std::cout << "Running simulation...\n";

    bool all_completed = gen.run_until_complete(10'000'000'000);  // 10 seconds max

    if (all_completed) {
        std::cout << "All requests completed successfully!\n";
    } else {
        std::cout << "Warning: Simulation timeout\n";
    }

    std::cout << "\n";

    // =========================================================================
    // 5. Print Statistics
    // =========================================================================

    std::cout << "=== Statistics ===\n\n";
    gen.print_statistics();

    auto stats = gen.get_statistics();

    std::cout << "\nKey Metrics:\n";
    std::cout << "  Total Requests: " << stats.total_requests << "\n";
    std::cout << "  Cache Hit Rate: " << std::fixed << std::setprecision(2)
              << (stats.hit_rate * 100.0) << "%\n";
    std::cout << "  Prefetch Accuracy: " << (stats.prefetch_accuracy * 100.0) << "%\n";
    std::cout << "  Avg Latency: " << stats.avg_latency_ns << " ns ("
              << (stats.avg_latency_ns / 1000.0) << " μs)\n";
    std::cout << "  Flash Reads: " << stats.flash_reads << "\n";
    std::cout << "  Flash Writes: " << stats.flash_writes << "\n";

    // =========================================================================
    // 6. Additional Test: Random Access Pattern
    // =========================================================================

    std::cout << "\n=== Testing Random Access Pattern ===\n";

    gen.reset_statistics();

    std::cout << "Submitting 50 random read requests...\n";

    for (int i = 0; i < 50; i++) {
        uint64_t address = (rand() % 10000) * 4096;  // Random address
        gen.submit_read(address, 4096);
    }

    gen.run_until_complete();

    auto stats2 = gen.get_statistics();
    std::cout << "  Cache Hit Rate (Random): " << (stats2.hit_rate * 100.0) << "%\n";
    std::cout << "  Avg Latency (Random): " << stats2.avg_latency_ns << " ns\n";

    std::cout << "\nDone!\n";

    return 0;
}
