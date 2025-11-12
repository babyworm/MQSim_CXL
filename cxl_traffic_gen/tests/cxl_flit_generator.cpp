/**
 * CXL Flit Generator - Generate and dump all CXL flit types
 *
 * This program generates sample flits for each CXL opcode type
 * and dumps them in hexadecimal format for analysis and debugging.
 */

#include "cxl_flit.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

using namespace CXL;

// Helper function to generate sample data
void generate_sample_data(uint8_t* buffer, size_t size, uint64_t seed = 0xABCDEF) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)((seed + i * 13) & 0xFF);
    }
}

// Structure to hold flit type information
struct FlitTypeInfo {
    std::string name;
    std::string description;
    CXLFlit flit;
};

void print_separator() {
    std::cout << "\n" << std::string(80, '=') << "\n";
}

void print_flit_info(const FlitTypeInfo& info) {
    print_separator();
    std::cout << "FLIT TYPE: " << info.name << "\n";
    std::cout << "Description: " << info.description << "\n";
    print_separator();
    info.flit.dump_header();
    info.flit.dump_hex(info.name);
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    std::string filter_type = "";
    if (argc >= 2) {
        filter_type = argv[1];
    }

    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                    CXL Flit Generator - All Flit Types                        \n";
    std::cout << "                         CXL 2.0 256-Byte Flits                                \n";
    std::cout << "================================================================================\n";
    std::cout << "\nUsage: " << argv[0] << " [flit_type]\n";
    std::cout << "  If flit_type is specified, only that type will be generated.\n";
    std::cout << "  Available types: mem_rd, mem_rd_data, mem_wr, mem_wr_ptl, mem_data,\n";
    std::cout << "                   mem_data_nxm, cpl, cpl_data, snp_data, snp_inv, all\n\n";

    // Sample data buffers
    uint8_t sample_data[CXLFlit::DATA_SIZE];
    generate_sample_data(sample_data, CXLFlit::DATA_SIZE, 0x12345678);

    // Define all flit types
    std::vector<FlitTypeInfo> flit_types;

    // 1. MEM_RD - Memory Read Request
    if (filter_type.empty() || filter_type == "mem_rd" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "MEM_RD (Memory Read)";
        info.description = "Request to read data from memory at specified address";
        info.flit = CXLFlit::create_mem_read(0x1000000, 4096, 0x100);
        flit_types.push_back(info);
    }

    // 2. MEM_RD_DATA - Memory Read with Data
    if (filter_type.empty() || filter_type == "mem_rd_data" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "MEM_RD_DATA (Memory Read with Data)";
        info.description = "Memory read request that includes data in the same flit";
        info.flit = CXLFlit::create_mem_read_data(0x2000000, sample_data, 240, 0x101);
        flit_types.push_back(info);
    }

    // 3. MEM_WR - Memory Write Request
    if (filter_type.empty() || filter_type == "mem_wr" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "MEM_WR (Memory Write)";
        info.description = "Request to write data to memory at specified address";
        info.flit = CXLFlit::create_mem_write(0x3000000, sample_data, 128, 0x102);
        flit_types.push_back(info);
    }

    // 4. MEM_WR_PTL - Memory Write Partial
    if (filter_type.empty() || filter_type == "mem_wr_ptl" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "MEM_WR_PTL (Memory Write Partial)";
        info.description = "Partial memory write (less than cache line size)";
        info.flit = CXLFlit::create_mem_write_partial(0x4000000, sample_data, 32, 0x103);
        flit_types.push_back(info);
    }

    // 5. MEM_DATA - Memory Data Response
    if (filter_type.empty() || filter_type == "mem_data" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "MEM_DATA (Memory Data Response)";
        info.description = "Response containing requested memory data";
        info.flit = CXLFlit::create_mem_data_response(0x5000000, sample_data, 240, 0x104);
        flit_types.push_back(info);
    }

    // 6. MEM_DATA_NXM - Memory Data Non-Existent Memory
    if (filter_type.empty() || filter_type == "mem_data_nxm" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "MEM_DATA_NXM (Non-Existent Memory)";
        info.description = "Response indicating requested memory does not exist";
        info.flit = CXLFlit::create_mem_data_nxm(0x6000000, 0x105);
        flit_types.push_back(info);
    }

    // 7. CPL - Completion (no data)
    if (filter_type.empty() || filter_type == "cpl" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "CPL (Completion)";
        info.description = "Completion acknowledgment without data";
        info.flit = CXLFlit::create_completion(0x106);
        flit_types.push_back(info);
    }

    // 8. CPL_DATA - Completion with Data
    if (filter_type.empty() || filter_type == "cpl_data" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "CPL_DATA (Completion with Data)";
        info.description = "Completion acknowledgment with associated data";
        info.flit = CXLFlit::create_completion_with_data(sample_data, 64, 0x107);
        flit_types.push_back(info);
    }

    // 9. SNP_DATA - Snoop Data (CXL.cache)
    if (filter_type.empty() || filter_type == "snp_data" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "SNP_DATA (Snoop Data)";
        info.description = "Cache coherency snoop with data response";
        info.flit = CXLFlit::create_snoop_data(0x7000000, sample_data, 64, 0x108);
        flit_types.push_back(info);
    }

    // 10. SNP_INV - Snoop Invalidate (CXL.cache)
    if (filter_type.empty() || filter_type == "snp_inv" || filter_type == "all") {
        FlitTypeInfo info;
        info.name = "SNP_INV (Snoop Invalidate)";
        info.description = "Cache coherency snoop to invalidate cache line";
        info.flit = CXLFlit::create_snoop_invalidate(0x8000000, 0x109);
        flit_types.push_back(info);
    }

    // Generate and print all flits
    if (flit_types.empty()) {
        std::cerr << "Error: Unknown flit type '" << filter_type << "'\n";
        return 1;
    }

    for (const auto& info : flit_types) {
        print_flit_info(info);
    }

    print_separator();
    std::cout << "Total Flit Types Generated: " << flit_types.size() << "\n";
    print_separator();
    std::cout << "\n";

    return 0;
}
