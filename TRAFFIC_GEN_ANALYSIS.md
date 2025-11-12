# CXL Traffic Generator - Component Analysis

## 필수 컴포넌트 (Keep)

### 1. CXL Layer (핵심)
```
src/cxl/
├── CXL_Manager              ✅ 핵심 - 요청 처리, cache/prefetch 결정
├── DRAM_Subsystem.h/cpp     ✅ 필수 - DRAM cache
├── CXL_MSHR.h/cpp           ✅ 필수 - Hit-under-miss
├── DRAM_Model.h/cpp         ✅ 필수 - DRAM timing
├── Prefetching_Alg.h/cpp    ✅ 필수 - Prefetcher
├── CXL_Config.h/cpp         ✅ 필수 - Configuration
├── CFLRU.h/cpp              ✅ 필수 - Cache policy
├── lrfu_heap.h/cpp          ✅ 필수 - Cache policy
└── OutputLog.h              ✅ 필수 - Logging
```

### 2. SSD/FTL Layer
```
src/ssd/
├── NVM_Firmware.h/cpp                    ✅ 필수 - FTL 코어
├── Address_Mapping_Unit_*.h/cpp          ✅ 필수 - LPA→PPA 변환
├── Flash_Block_Manager.h/cpp             ✅ 필수 - 블록 관리
├── GC_and_WL_Unit_*.h/cpp                ✅ 필수 - GC
├── TSU_*.h/cpp                           ✅ 필수 - Transaction 스케줄링
├── NVM_PHY_*.h/cpp                       ✅ 필수 - Physical layer
├── ONFI_Channel_*.h/cpp                  ✅ 필수 - Channel
├── NVM_Transaction_*.h/cpp               ✅ 필수 - Transaction 정의
├── Data_Cache_Manager_*.h/cpp            ⚠️  선택 - SSD data cache (CXL cache 있으면 덜 중요)
└── SSD_Defs.h                            ✅ 필수 - Type definitions
```

### 3. Flash Chip Layer
```
src/nvm_chip/flash_memory/
├── Flash_Chip.h/cpp              ✅ 필수
├── Die.h/cpp                     ✅ 필수
├── Plane.h/cpp                   ✅ 필수
├── Block.h/cpp                   ✅ 필수
├── Page.h                        ✅ 필수
├── Physical_Page_Address.h/cpp   ✅ 필수
└── Flash_Command.h               ✅ 필수
```

### 4. Simulation Engine
```
src/sim/
├── Engine.h/cpp        ✅ 필수 - DES 엔진
├── EventTree.h/cpp     ✅ 필수 - Event queue
├── Sim_Object.h        ✅ 필수 - Base class
├── Sim_Event.h         ✅ 필수 - Event 정의
├── Sim_Reporter.h      ✅ 필수 - Statistics
└── Sim_Defs.h          ✅ 필수 - Type definitions
```

### 5. Utilities
```
src/utils/
├── Logical_Address_Partitioning_Unit.h/cpp  ✅ 필수
├── DistributionTypes.h                      ⚠️  선택
├── RandomGenerator.h/cpp                    ⚠️  선택
├── XMLWriter.h/cpp                          ⚠️  선택 (간단한 logging으로 대체 가능)
└── rapidxml/                                ⚠️  선택 (XML config 제거 가능)
```

---

## 제거할 컴포넌트 (Remove)

### 1. Host System (전체 제거)
```
src/host/
├── IO_Flow_Base.h/cpp              ❌ 제거
├── IO_Flow_Trace_Based.h/cpp       ❌ 제거
├── IO_Flow_Synthetic.h/cpp         ❌ 제거
├── PCIe_Root_Complex.h/cpp         ❌ 제거
├── PCIe_Switch.h/cpp                ❌ 제거
├── PCIe_Link.h/cpp                  ❌ 제거
├── PCIe_Message.h                   ❌ 제거
└── SATA_HBA.h/cpp                   ❌ 제거
```

### 2. 불필요한 Host Interface
```
src/ssd/
├── Host_Interface_NVMe.h/cpp    ❌ 제거
├── Host_Interface_SATA.h/cpp    ❌ 제거
└── Host_Interface_Defs.h        ⚠️  일부 type만 유지
```

### 3. Execution/Parameter 관리
```
src/exec/
├── Host_System.h/cpp                ❌ 제거
├── Execution_Parameter_Set.h/cpp    ❌ 제거 (간단한 struct로 대체)
├── Host_Parameter_Set.h/cpp         ❌ 제거
├── IO_Flow_Parameter_Set.h/cpp      ❌ 제거
└── Device_Parameter_Set.h/cpp       ⚠️  간소화 (struct로 대체)
```

### 4. 기타
```
src/main.cpp                 ❌ 제거 (새로운 API로 대체)
config.txt                   ⚠️  간소화
ssdconfig.xml                ⚠️  간소화 또는 제거
workload.xml                 ❌ 제거
```

---

## 새로운 구조

```
cxl_traffic_gen/
├── include/
│   └── cxl_traffic_gen.h         # Public API
├── src/
│   ├── core/
│   │   ├── traffic_generator.h/cpp
│   │   └── request.h
│   ├── cxl/                      # CXL layer (기존 파일들)
│   ├── ssd/                      # SSD layer (필수 파일만)
│   ├── flash/                    # Flash chip (기존 nvm_chip)
│   ├── sim/                      # Simulation engine
│   └── utils/                    # Utilities (최소화)
├── examples/
│   ├── standalone_example.cpp
│   └── gem5_integration_example.cpp
├── config/
│   └── default_config.h          # C++ struct config
├── CMakeLists.txt
└── README.md
```

---

## 새로운 API 설계

```cpp
// include/cxl_traffic_gen.h

class CXLTrafficGenerator {
public:
    // Configuration
    struct Config {
        // CXL parameters
        uint64_t dram_size;
        std::string cache_policy;
        std::string prefetcher;
        bool has_mshr;
        uint16_t set_associativity;

        // Flash parameters
        uint32_t num_channels;
        uint32_t chips_per_channel;
        uint64_t page_read_latency_ns;
        uint64_t page_program_latency_ns;
        uint64_t block_erase_latency_ns;
    };

    // Constructor
    CXLTrafficGenerator(const Config& config);

    // Request submission
    using CompletionCallback = std::function<void(uint64_t request_id, uint64_t latency_ns)>;

    uint64_t submit_read(uint64_t address, uint32_t size, CompletionCallback callback);
    uint64_t submit_write(uint64_t address, uint32_t size, const void* data, CompletionCallback callback);

    // Simulation control
    void tick(uint64_t time_ns);          // Advance simulation
    void run_until(uint64_t time_ns);     // Run until time
    void run_until_complete();            // Run until all requests complete

    // Statistics
    struct Statistics {
        uint64_t cache_hits;
        uint64_t cache_misses;
        double hit_rate;
        uint64_t prefetch_hits;
        uint64_t flash_reads;
        uint64_t flash_writes;
        double avg_latency_ns;
    };

    Statistics get_statistics() const;
    void reset_statistics();

private:
    // Internal components
    std::unique_ptr<CXL_Manager> cxl_manager_;
    std::unique_ptr<SSD_Device> ssd_device_;
    // ...
};
```

---

## 사용 예제

### Standalone
```cpp
#include "cxl_traffic_gen.h"

int main() {
    // Configure
    CXLTrafficGenerator::Config config;
    config.dram_size = 64 * 1024 * 1024;  // 64 MB
    config.cache_policy = "CFLRU";
    config.prefetcher = "Best-offset";

    // Create generator
    CXLTrafficGenerator gen(config);

    // Submit requests
    gen.submit_read(0x1000, 4096, [](uint64_t id, uint64_t latency) {
        std::cout << "Request " << id << " completed in " << latency << " ns\n";
    });

    // Run simulation
    gen.run_until_complete();

    // Print statistics
    auto stats = gen.get_statistics();
    std::cout << "Hit rate: " << stats.hit_rate << "\n";
}
```

### Gem5 Integration
```cpp
// In Gem5 SimObject
class CXLFlashDevice : public SimObject {
    CXLTrafficGenerator* traffic_gen_;

    bool recvTimingReq(PacketPtr pkt) override {
        uint64_t addr = pkt->getAddr();
        uint32_t size = pkt->getSize();

        traffic_gen_->submit_read(addr, size, [this, pkt](uint64_t id, uint64_t latency) {
            // Convert latency to Gem5 ticks
            Tick ticks = SimClock::Int::ns * latency;
            schedule(pkt, curTick() + ticks);
        });

        return true;
    }

    void tick() {
        // Advance traffic generator
        traffic_gen_->tick(curTick() / SimClock::Int::ns);
    }
};
```

---

## 다음 단계

1. ✅ 컴포넌트 분석 완료
2. ⏭️ 새 디렉토리 생성 및 파일 복사
3. ⏭️ CXLTrafficGenerator 클래스 구현
4. ⏭️ 예제 코드 작성
5. ⏭️ CMake 빌드 시스템 설정
6. ⏭️ README 및 문서 작성
