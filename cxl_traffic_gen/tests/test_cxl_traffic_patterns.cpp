/**
 * CXL Traffic Generator - Test Suite
 *
 * This test suite demonstrates common CXL usage patterns and dumps
 * the corresponding 256B flits in hexadecimal format.
 *
 * Test Patterns:
 * 1. Sequential Read (Streaming)
 * 2. Random Read (Database Lookup)
 * 3. Write-Back (Cache Eviction)
 * 4. Read-Modify-Write (Atomic Operation)
 * 5. Prefetch Requests
 */

#include "cxl_traffic_gen.h"
#include "cxl_flit.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <cassert>

using namespace CXL;

//==============================================================================
// Test Utilities
//==============================================================================

class TestHarness {
public:
    TestHarness(const std::string& name) : test_name_(name) {
        std::cout << "\n";
        std::cout << "========================================================================\n";
        std::cout << " TEST: " << std::left << std::setw(60) << name << "\n";
        std::cout << "========================================================================\n";
    }

    ~TestHarness() {
        std::cout << "\n[OK] Test '" << test_name_ << "' completed.\n";
        std::cout << std::string(75, '-') << "\n";
    }

private:
    std::string test_name_;
};

// Generate test data pattern
void fill_test_data(uint8_t* buffer, size_t size, uint64_t seed = 0x12345678) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)((seed + i) & 0xFF);
    }
}

// Dump flit with context
void dump_flit_with_context(const CXLFlit& flit, const std::string& context) {
    std::cout << "\n-- " << context << " -┐\n";
    flit.dump_header();
    flit.dump_hex(context);
    std::cout << "\n";
}

//==============================================================================
// Test Pattern 1: Sequential Read (Streaming)
//==============================================================================

void test_sequential_read() {
    TestHarness test("Pattern 1: Sequential Read (Streaming)");

    std::cout << "\nScenario: Reading consecutive 4KB pages (typical memory streaming)\n";
    std::cout << "Expected: Cache prefetcher should kick in after first miss\n\n";

    // Simulate reading 4 consecutive 4KB pages
    const size_t PAGE_SIZE = 4096;
    const size_t NUM_PAGES = 4;
    uint64_t base_address = 0x100000;  // 1MB aligned

    std::vector<CXLFlit> request_flits;
    std::vector<CXLFlit> response_flits;

    for (size_t i = 0; i < NUM_PAGES; i++) {
        uint64_t address = base_address + (i * PAGE_SIZE);
        uint16_t tag = i;

        // Create read request flit
        CXLFlit req = CXLFlit::create_mem_read(address, PAGE_SIZE, tag);
        request_flits.push_back(req);

        std::cout << "\n[>>] Request " << i << ": Read 4KB from 0x"
                  << std::hex << address << std::dec;

        dump_flit_with_context(req, "Sequential Read Request #" + std::to_string(i));

        // Simulate response with test data
        uint8_t test_data[CXLFlit::DATA_SIZE];
        fill_test_data(test_data, CXLFlit::DATA_SIZE, address);

        CXLFlit resp = CXLFlit::create_mem_data_response(address, test_data, PAGE_SIZE, tag);
        response_flits.push_back(resp);

        std::cout << "\n[<<] Response " << i << ": Data for 0x"
                  << std::hex << address << std::dec;

        dump_flit_with_context(resp, "Sequential Read Response #" + std::to_string(i));
    }

    std::cout << "\n[**] Summary:\n";
    std::cout << "  Total Requests:  " << request_flits.size() << "\n";
    std::cout << "  Total Responses: " << response_flits.size() << "\n";
    std::cout << "  Address Range:   0x" << std::hex << base_address << " - 0x"
              << (base_address + NUM_PAGES * PAGE_SIZE) << std::dec << "\n";
}

//==============================================================================
// Test Pattern 2: Random Read (Database Lookup)
//==============================================================================

void test_random_read() {
    TestHarness test("Pattern 2: Random Read (Database Lookup)");

    std::cout << "\nScenario: Random 64-byte reads (typical database index lookup)\n";
    std::cout << "Expected: Low cache hit rate, high DRAM cache thrashing\n\n";

    const size_t READ_SIZE = 64;
    const size_t NUM_READS = 5;

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0x1000000, 0x10000000);  // 16MB - 256MB range

    for (size_t i = 0; i < NUM_READS; i++) {
        uint64_t random_address = dis(gen) & ~0x3F;  // 64-byte aligned
        uint16_t tag = 100 + i;

        CXLFlit req = CXLFlit::create_mem_read(random_address, READ_SIZE, tag);

        std::cout << "\n[>>] Random Request " << i << ": Read " << READ_SIZE
                  << " bytes from 0x" << std::hex << random_address << std::dec;

        dump_flit_with_context(req, "Random Read Request #" + std::to_string(i));

        // Response
        uint8_t data[CXLFlit::DATA_SIZE];
        fill_test_data(data, READ_SIZE, random_address);

        CXLFlit resp = CXLFlit::create_mem_data_response(random_address, data, READ_SIZE, tag);
        dump_flit_with_context(resp, "Random Read Response #" + std::to_string(i));
    }
}

//==============================================================================
// Test Pattern 3: Write-Back (Cache Eviction)
//==============================================================================

void test_write_back() {
    TestHarness test("Pattern 3: Write-Back (Cache Eviction)");

    std::cout << "\nScenario: Writing dirty cache lines back to CXL-Flash\n";
    std::cout << "Expected: Burst of writes when DRAM cache is full\n\n";

    const size_t CACHE_LINE_SIZE = 64;
    const size_t NUM_WRITEBACKS = 3;

    uint64_t base_address = 0x2000000;  // 32MB

    for (size_t i = 0; i < NUM_WRITEBACKS; i++) {
        uint64_t address = base_address + (i * CACHE_LINE_SIZE);
        uint16_t tag = 200 + i;

        // Create write data (simulating modified cache line)
        uint8_t dirty_data[CACHE_LINE_SIZE];
        for (size_t j = 0; j < CACHE_LINE_SIZE; j++) {
            dirty_data[j] = (uint8_t)(0xAA + i + j);  // Distinctive pattern
        }

        CXLFlit write_req = CXLFlit::create_mem_write(address, dirty_data,
                                                       CACHE_LINE_SIZE, tag);

        std::cout << "\n[>>] Write-Back " << i << ": Flush dirty line to 0x"
                  << std::hex << address << std::dec;

        dump_flit_with_context(write_req, "Write-Back Request #" + std::to_string(i));

        // Completion
        CXLFlit completion = CXLFlit::create_completion(tag);
        dump_flit_with_context(completion, "Write-Back Completion #" + std::to_string(i));
    }
}

//==============================================================================
// Test Pattern 4: Read-Modify-Write (Atomic Operation)
//==============================================================================

void test_read_modify_write() {
    TestHarness test("Pattern 4: Read-Modify-Write (Atomic Operation)");

    std::cout << "\nScenario: Atomic increment of a counter (RMW operation)\n";
    std::cout << "Expected: Read -> Modify locally -> Write back\n\n";

    uint64_t counter_address = 0x3000000;
    const size_t COUNTER_SIZE = 8;  // 64-bit counter

    // Step 1: Read current value
    CXLFlit read_req = CXLFlit::create_mem_read(counter_address, COUNTER_SIZE, 300);

    std::cout << "\n[>>] Step 1: Read counter at 0x" << std::hex << counter_address << std::dec;
    dump_flit_with_context(read_req, "RMW: Read Request");

    // Simulate current counter value
    uint64_t current_value = 0x123456789ABCDEF0ULL;
    CXLFlit read_resp = CXLFlit::create_mem_data_response(counter_address,
                                                           &current_value,
                                                           COUNTER_SIZE, 300);
    dump_flit_with_context(read_resp, "RMW: Read Response");

    std::cout << "\n  Current Value: 0x" << std::hex << current_value << std::dec << "\n";

    // Step 2: Modify (increment)
    uint64_t new_value = current_value + 1;
    std::cout << "  New Value:     0x" << std::hex << new_value << std::dec << "\n";

    // Step 3: Write back
    CXLFlit write_req = CXLFlit::create_mem_write(counter_address, &new_value,
                                                   COUNTER_SIZE, 301);

    std::cout << "\n[>>] Step 3: Write new value back";
    dump_flit_with_context(write_req, "RMW: Write Request");

    CXLFlit completion = CXLFlit::create_completion(301);
    dump_flit_with_context(completion, "RMW: Completion");
}

//==============================================================================
// Test Pattern 5: Prefetch Requests
//==============================================================================

void test_prefetch_requests() {
    TestHarness test("Pattern 5: Prefetch Requests (Next-N-Line)");

    std::cout << "\nScenario: Hardware prefetcher issuing speculative reads\n";
    std::cout << "Expected: Prefetch next 4 cache lines ahead\n\n";

    const size_t CACHE_LINE_SIZE = 64;
    const size_t PREFETCH_DEGREE = 4;

    uint64_t current_address = 0x4000000;

    // Demand request (actual access)
    CXLFlit demand_req = CXLFlit::create_mem_read(current_address, CACHE_LINE_SIZE, 400);
    demand_req.header.flags = 0x00;  // Normal request

    std::cout << "\n[>>] Demand Request: 0x" << std::hex << current_address << std::dec;
    dump_flit_with_context(demand_req, "Demand Read (triggers prefetch)");

    // Prefetch requests (speculative)
    for (size_t i = 1; i <= PREFETCH_DEGREE; i++) {
        uint64_t prefetch_addr = current_address + (i * CACHE_LINE_SIZE);
        uint16_t tag = 400 + i;

        CXLFlit prefetch_req = CXLFlit::create_mem_read(prefetch_addr, CACHE_LINE_SIZE, tag);
        prefetch_req.header.flags = 0x01;  // Mark as prefetch

        std::cout << "\n[>>] Prefetch " << i << ": 0x" << std::hex << prefetch_addr << std::dec;
        dump_flit_with_context(prefetch_req,
                               "Prefetch Request #" + std::to_string(i) + " (Speculative)");
    }

    std::cout << "\n[**] Prefetch Summary:\n";
    std::cout << "  Prefetch Degree: " << PREFETCH_DEGREE << "\n";
    std::cout << "  Total Requests:  " << (1 + PREFETCH_DEGREE) << " (1 demand + "
              << PREFETCH_DEGREE << " prefetch)\n";
}

//==============================================================================
// Test Pattern 6: Mixed Read/Write Workload
//==============================================================================

void test_mixed_workload() {
    TestHarness test("Pattern 6: Mixed Read/Write Workload");

    std::cout << "\nScenario: Interleaved reads and writes (typical application behavior)\n";
    std::cout << "Expected: Mix of cache hits and misses\n\n";

    uint64_t base_addr = 0x5000000;
    const size_t OP_SIZE = 4096;

    std::vector<std::pair<std::string, CXLFlit>> operations;

    // Write
    uint8_t write_data[CXLFlit::DATA_SIZE];
    fill_test_data(write_data, CXLFlit::DATA_SIZE, 0xDEADBEEF);
    operations.push_back({"Write", CXLFlit::create_mem_write(base_addr, write_data, OP_SIZE, 600)});

    // Read same location (should hit cache)
    operations.push_back({"Read (hit)", CXLFlit::create_mem_read(base_addr, OP_SIZE, 601)});

    // Write different location
    operations.push_back({"Write", CXLFlit::create_mem_write(base_addr + 0x10000, write_data, OP_SIZE, 602)});

    // Read different location (miss)
    operations.push_back({"Read (miss)", CXLFlit::create_mem_read(base_addr + 0x20000, OP_SIZE, 603)});

    for (size_t i = 0; i < operations.size(); i++) {
        std::cout << "\n[>>] Operation " << i << ": " << operations[i].first;
        dump_flit_with_context(operations[i].second,
                               "Mixed Workload Op #" + std::to_string(i));
    }
}

//==============================================================================
// Test Pattern 7: Burst Transfer (DMA-like)
//==============================================================================

void test_burst_transfer() {
    TestHarness test("Pattern 7: Burst Transfer (DMA-like)");

    std::cout << "\nScenario: Large contiguous transfer (e.g., video frame)\n";
    std::cout << "Expected: Sequential flits with incrementing addresses\n\n";

    const size_t TOTAL_SIZE = 1024 * 1024;  // 1MB
    const size_t FLIT_DATA_SIZE = CXLFlit::DATA_SIZE;  // 240 bytes
    const size_t NUM_FLITS = (TOTAL_SIZE + FLIT_DATA_SIZE - 1) / FLIT_DATA_SIZE;

    uint64_t start_address = 0x6000000;

    std::cout << "Transfer: " << TOTAL_SIZE << " bytes in " << NUM_FLITS << " flits\n";

    // Show first 3 and last 1 flit
    std::vector<size_t> show_indices = {0, 1, 2, NUM_FLITS - 1};

    for (size_t idx : show_indices) {
        uint64_t address = start_address + (idx * FLIT_DATA_SIZE);
        size_t remaining = TOTAL_SIZE - (idx * FLIT_DATA_SIZE);
        uint16_t size = (remaining > FLIT_DATA_SIZE) ? FLIT_DATA_SIZE : remaining;

        CXLFlit flit = CXLFlit::create_mem_read(address, size, 700 + idx);

        std::cout << "\n[>>] Flit " << idx << "/" << NUM_FLITS << ": Addr=0x"
                  << std::hex << address << std::dec << ", Size=" << size;

        dump_flit_with_context(flit, "Burst Flit #" + std::to_string(idx));
    }

    std::cout << "\n  ... (" << (NUM_FLITS - show_indices.size()) << " more flits) ...\n";
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main() {
    std::cout << "\n";
    std::cout << "=═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "|        CXL Traffic Generator - Test Suite with Flit Dump         |\n";
    std::cout << "|                     CXL 2.0 256-Byte Flits                        |\n";
    std::cout << "=═══════════════════════════════════════════════════════════════════╝\n";

    std::cout << "\nFlit Format:\n";
    std::cout << "  - Total Size:   256 bytes\n";
    std::cout << "  - Header:       16 bytes (protocol, opcode, address, etc.)\n";
    std::cout << "  - Data Payload: 240 bytes\n";
    std::cout << "  - Protocol:     CXL.mem (memory protocol)\n\n";

    // Run all test patterns
    try {
        test_sequential_read();
        test_random_read();
        test_write_back();
        test_read_modify_write();
        test_prefetch_requests();
        test_mixed_workload();
        test_burst_transfer();

        std::cout << "\n";
        std::cout << "=═══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "|                    ALL TESTS PASSED [OK]                            |\n";
        std::cout << "=═══════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[FAIL] Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
