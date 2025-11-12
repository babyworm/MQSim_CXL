#ifndef CXL_FLIT_H
#define CXL_FLIT_H

#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace CXL {

/**
 * CXL 2.0+ 256-Byte Flit Structure
 *
 * A flit (Flow Control Unit) is the fundamental transfer unit in CXL.
 * CXL 2.0 supports 256B flits for improved bandwidth.
 */

//==============================================================================
// CXL.mem Protocol Opcodes
//==============================================================================

enum class CXLMemOpcode : uint8_t {
    // Memory Read Requests
    MEM_RD              = 0x00,     // Memory Read
    MEM_RD_DATA         = 0x01,     // Memory Read (with data)

    // Memory Write Requests
    MEM_WR              = 0x10,     // Memory Write
    MEM_WR_PTL          = 0x11,     // Memory Write Partial

    // Memory Response
    MEM_DATA            = 0x20,     // Memory Data Response
    MEM_DATA_NXM        = 0x21,     // Memory Data (Non-Existent Memory)

    // Completion
    CPL                 = 0x30,     // Completion (no data)
    CPL_DATA            = 0x31,     // Completion with data

    // Cache-related (CXL.cache)
    SNP_DATA            = 0x40,     // Snoop Data
    SNP_INV             = 0x41,     // Snoop Invalidate

    // Misc
    RESERVED            = 0xFF
};

//==============================================================================
// CXL Flit Header (16 bytes)
//==============================================================================

struct CXLFlitHeader {
    // Byte 0-1: Protocol and Opcode
    uint8_t protocol_id;            // Protocol ID (0x0=CXL.io, 0x1=CXL.cache, 0x2=CXL.mem)
    CXLMemOpcode opcode;            // Operation code

    // Byte 2-3: Transaction metadata
    uint16_t tag;                   // Transaction tag/ID

    // Byte 4-11: Address (64-bit)
    uint64_t address;               // Physical address (byte-addressable)

    // Byte 12-13: Size and metadata
    uint16_t length;                // Data length in bytes

    // Byte 14-15: Reserved / Metadata
    uint8_t cache_id;               // Cache ID (for multi-level)
    uint8_t flags;                  // Various flags

    CXLFlitHeader() {
        memset(this, 0, sizeof(CXLFlitHeader));
        protocol_id = 0x2;  // CXL.mem by default
    }
} __attribute__((packed));

static_assert(sizeof(CXLFlitHeader) == 16, "CXL Flit Header must be 16 bytes");

//==============================================================================
// CXL Flit (256 bytes total)
//==============================================================================

struct CXLFlit {
    static constexpr size_t FLIT_SIZE = 256;
    static constexpr size_t HEADER_SIZE = 16;
    static constexpr size_t DATA_SIZE = 240;  // 256 - 16 = 240 bytes

    CXLFlitHeader header;           // 16 bytes
    uint8_t data[DATA_SIZE];        // 240 bytes

    CXLFlit() {
        memset(data, 0, DATA_SIZE);
    }

    // Factory methods for common operations

    static CXLFlit create_mem_read(uint64_t address, uint16_t size, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;  // CXL.mem
        flit.header.opcode = CXLMemOpcode::MEM_RD;
        flit.header.address = address;
        flit.header.length = size;
        flit.header.tag = tag;
        return flit;
    }

    static CXLFlit create_mem_write(uint64_t address, const void* write_data,
                                     uint16_t size, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;
        flit.header.opcode = CXLMemOpcode::MEM_WR;
        flit.header.address = address;
        flit.header.length = size;
        flit.header.tag = tag;

        // Copy data (up to DATA_SIZE)
        size_t copy_size = (size > DATA_SIZE) ? DATA_SIZE : size;
        if (write_data) {
            memcpy(flit.data, write_data, copy_size);
        }

        return flit;
    }

    static CXLFlit create_mem_data_response(uint64_t address, const void* resp_data,
                                            uint16_t size, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;
        flit.header.opcode = CXLMemOpcode::MEM_DATA;
        flit.header.address = address;
        flit.header.length = size;
        flit.header.tag = tag;

        size_t copy_size = (size > DATA_SIZE) ? DATA_SIZE : size;
        if (resp_data) {
            memcpy(flit.data, resp_data, copy_size);
        }

        return flit;
    }

    static CXLFlit create_completion(uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;
        flit.header.opcode = CXLMemOpcode::CPL;
        flit.header.tag = tag;
        return flit;
    }

    static CXLFlit create_mem_read_data(uint64_t address, const void* read_data,
                                        uint16_t size, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;
        flit.header.opcode = CXLMemOpcode::MEM_RD_DATA;
        flit.header.address = address;
        flit.header.length = size;
        flit.header.tag = tag;

        size_t copy_size = (size > DATA_SIZE) ? DATA_SIZE : size;
        if (read_data) {
            memcpy(flit.data, read_data, copy_size);
        }
        return flit;
    }

    static CXLFlit create_mem_write_partial(uint64_t address, const void* write_data,
                                            uint16_t size, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;
        flit.header.opcode = CXLMemOpcode::MEM_WR_PTL;
        flit.header.address = address;
        flit.header.length = size;
        flit.header.tag = tag;

        size_t copy_size = (size > DATA_SIZE) ? DATA_SIZE : size;
        if (write_data) {
            memcpy(flit.data, write_data, copy_size);
        }
        return flit;
    }

    static CXLFlit create_mem_data_nxm(uint64_t address, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;
        flit.header.opcode = CXLMemOpcode::MEM_DATA_NXM;
        flit.header.address = address;
        flit.header.tag = tag;
        return flit;
    }

    static CXLFlit create_completion_with_data(const void* cpl_data, uint16_t size,
                                                uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x2;
        flit.header.opcode = CXLMemOpcode::CPL_DATA;
        flit.header.length = size;
        flit.header.tag = tag;

        size_t copy_size = (size > DATA_SIZE) ? DATA_SIZE : size;
        if (cpl_data) {
            memcpy(flit.data, cpl_data, copy_size);
        }
        return flit;
    }

    static CXLFlit create_snoop_data(uint64_t address, const void* snoop_data,
                                     uint16_t size, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x1;  // CXL.cache
        flit.header.opcode = CXLMemOpcode::SNP_DATA;
        flit.header.address = address;
        flit.header.length = size;
        flit.header.tag = tag;

        size_t copy_size = (size > DATA_SIZE) ? DATA_SIZE : size;
        if (snoop_data) {
            memcpy(flit.data, snoop_data, copy_size);
        }
        return flit;
    }

    static CXLFlit create_snoop_invalidate(uint64_t address, uint16_t tag = 0) {
        CXLFlit flit;
        flit.header.protocol_id = 0x1;  // CXL.cache
        flit.header.opcode = CXLMemOpcode::SNP_INV;
        flit.header.address = address;
        flit.header.tag = tag;
        return flit;
    }

    // Generic factory for any opcode
    static CXLFlit create_custom(uint8_t protocol_id, CXLMemOpcode opcode,
                                  uint64_t address, const void* payload_data,
                                  uint16_t size, uint16_t tag = 0,
                                  uint8_t cache_id = 0, uint8_t flags = 0) {
        CXLFlit flit;
        flit.header.protocol_id = protocol_id;
        flit.header.opcode = opcode;
        flit.header.address = address;
        flit.header.length = size;
        flit.header.tag = tag;
        flit.header.cache_id = cache_id;
        flit.header.flags = flags;

        if (payload_data && size > 0) {
            size_t copy_size = (size > DATA_SIZE) ? DATA_SIZE : size;
            memcpy(flit.data, payload_data, copy_size);
        }
        return flit;
    }

    // Hex dump utilities

    std::string to_hex_string(size_t bytes_per_line = 16) const {
        std::ostringstream oss;
        const uint8_t* raw = reinterpret_cast<const uint8_t*>(this);

        for (size_t i = 0; i < FLIT_SIZE; i++) {
            if (i % bytes_per_line == 0) {
                oss << "\n  " << std::hex << std::setw(4) << std::setfill('0') << i << ": ";
            }
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)raw[i] << " ";
        }

        return oss.str();
    }

    void dump_hex(const std::string& title = "CXL Flit") const {
        std::cout << "\n=== " << title << " (256 bytes) ===" << to_hex_string() << "\n";
    }

    void dump_header() const {
        // Save iostream state
        std::ios_base::fmtflags saved_flags = std::cout.flags();
        char saved_fill = std::cout.fill();

        std::cout << "\n--- CXL Flit Header ---\n";
        std::cout << "  Protocol ID: 0x" << std::hex << (int)header.protocol_id
                  << " (" << get_protocol_name() << ")\n";
        std::cout << "  Opcode:      0x" << std::hex << (int)header.opcode
                  << " (" << get_opcode_name() << ")\n";
        std::cout << "  Tag:         0x" << std::hex << header.tag << "\n";
        std::cout << "  Address:     0x" << std::setfill('0') << std::setw(16) << std::hex
                  << header.address << "\n";
        std::cout << "  Length:      " << std::dec << header.length << " bytes\n";
        std::cout << "  Cache ID:    " << std::dec << (int)header.cache_id << "\n";
        std::cout << "  Flags:       0x" << std::hex << (int)header.flags << "\n";

        // Restore iostream state
        std::cout.flags(saved_flags);
        std::cout.fill(saved_fill);
    }

    std::string get_protocol_name() const {
        switch (header.protocol_id) {
            case 0x0: return "CXL.io";
            case 0x1: return "CXL.cache";
            case 0x2: return "CXL.mem";
            default:  return "Unknown";
        }
    }

    std::string get_opcode_name() const {
        switch (header.opcode) {
            case CXLMemOpcode::MEM_RD:       return "MEM_RD";
            case CXLMemOpcode::MEM_RD_DATA:  return "MEM_RD_DATA";
            case CXLMemOpcode::MEM_WR:       return "MEM_WR";
            case CXLMemOpcode::MEM_WR_PTL:   return "MEM_WR_PTL";
            case CXLMemOpcode::MEM_DATA:     return "MEM_DATA";
            case CXLMemOpcode::MEM_DATA_NXM: return "MEM_DATA_NXM";
            case CXLMemOpcode::CPL:          return "CPL";
            case CXLMemOpcode::CPL_DATA:     return "CPL_DATA";
            case CXLMemOpcode::SNP_DATA:     return "SNP_DATA";
            case CXLMemOpcode::SNP_INV:      return "SNP_INV";
            default:                         return "RESERVED";
        }
    }

} __attribute__((packed));

static_assert(sizeof(CXLFlit) == 256, "CXL Flit must be 256 bytes");

//==============================================================================
// Helper Functions
//==============================================================================

inline void dump_memory_region(const void* ptr, size_t size,
                               const std::string& title = "Memory Region") {
    std::cout << "\n=== " << title << " (" << size << " bytes) ===";
    const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);

    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            std::cout << "\n  " << std::hex << std::setw(4) << std::setfill('0') << i << ": ";
        }
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    }
    std::cout << "\n" << std::dec;
}

} // namespace CXL

#endif // CXL_FLIT_H
