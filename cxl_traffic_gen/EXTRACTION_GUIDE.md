# CXL Traffic Generator - Extraction Guide

> 이 문서는 MQSim_CXL에서 traffic generator를 분리한 과정을 설명합니다.

---

## 분리 목적

MQSim_CXL의 **핵심 CXL 시뮬레이션 기능만 추출**하여:
1. **Gem5 연동 용이성**: Gem5에서 CXL traffic generator로 사용
2. **코드 간소화**: 불필요한 host system, trace 처리 제거
3. **독립 실행**: Standalone 라이브러리로 사용 가능

---

## 유지된 컴포넌트

### ✅ CXL Layer (src/cxl/)
- `CXL_Manager` - 핵심 request 처리
- `DRAM_Subsystem` - Device-side DRAM cache
- `CXL_MSHR` - Hit-under-miss 지원
- `DRAM_Model` - DRAM timing 시뮬레이션
- `Prefetching_Alg` - Prefetcher (Tagged, BO, LEAP)
- `CFLRU`, `lrfu_heap` - Cache replacement policies
- `CXL_Config` - Configuration parsing

### ✅ Simulation Engine (src/sim/)
- `Engine` - Discrete event simulator
- `EventTree` - Event priority queue
- `Sim_Object`, `Sim_Event` - Base classes
- `Sim_Reporter` - Statistics

### ✅ Flash Backend (src/ssd/, src/flash/)
- `NVM_Firmware` - FTL core
- `Address_Mapping_Unit` - LPA→PPA 변환
- `Flash_Block_Manager` - Block 관리
- `GC_and_WL_Unit` - Garbage collection + wear-leveling
- `TSU` - Transaction scheduling
- `NVM_PHY`, `ONFI_Channel` - Physical layer
- `Flash_Chip` - Flash hierarchy (Die/Plane/Block/Page)

---

## 제거된 컴포넌트

### ❌ Host System (src/host/)
- `IO_Flow_Trace_Based` - Trace file 처리
- `IO_Flow_Synthetic` - Synthetic workload 생성
- `PCIe_Root_Complex`, `PCIe_Switch` - PCIe infrastructure
- `SATA_HBA` - SATA support

**이유**: Traffic generator는 직접 API를 제공하므로 host system 불필요

### ❌ Configuration System (src/exec/)
- `Host_System`, `Host_Parameter_Set` - Host 관련 설정
- `Execution_Parameter_Set` - 복잡한 XML 기반 설정
- `IO_Flow_Parameter_Set` - Workload 설정

**이유**: 간단한 C++ struct로 대체

### ❌ Host Interfaces
- `Host_Interface_NVMe` - NVMe 프로토콜
- `Host_Interface_SATA` - SATA 프로토콜
- `Host_Interface_CXL`의 일부 - PCIe message 처리

**이유**: Public API로 대체

### ❌ 기타
- `main.cpp` - Standalone 실행 파일
- `workload.xml` - Workload 정의
- `ssdconfig.xml` - XML 기반 설정 (간소화)
- `config.txt` - Text 기반 설정 (간소화)

---

## 새로운 구조

```
cxl_traffic_gen/
├── include/
│   └── cxl_traffic_gen.h         # ⭐ Public API
│
├── src/
│   ├── core/
│   │   └── traffic_generator.cpp  # ⭐ Implementation (PIMPL pattern)
│   │
│   ├── cxl/                       # CXL layer (from src/cxl/)
│   ├── ssd/                       # SSD layer (filtered from src/ssd/)
│   ├── flash/                     # Flash chips (from src/nvm_chip/)
│   ├── sim/                       # Simulation engine (from src/sim/)
│   └── utils/                     # Minimal utilities
│
├── examples/
│   ├── standalone_example.cpp     # ⭐ Standalone usage
│   └── gem5_integration_example.cpp  # ⭐ Gem5 integration
│
├── config/
│   └── default_config.h           # Default configuration
│
├── CMakeLists.txt                 # ⭐ Build system
└── README.md                      # ⭐ Documentation
```

---

## 주요 변경사항

### 1. Public API 설계

**Before (MQSim_CXL)**:
```cpp
// 복잡한 초기화
SSD_Device* ssd = new SSD_Device(params, ...);
Host_System* host = new Host_System(...);
host->Attach_ssd_device(ssd);

// PCIe message 생성
PCIe_Message* msg = create_message(...);
ssd->Host_interface->Consume_pcie_message(msg);
```

**After (Traffic Generator)**:
```cpp
// 간단한 초기화
CXL::TrafficGenerator::Config config;
config.dram_size = 64 * 1024 * 1024;
CXL::TrafficGenerator gen(config);

// 직접 request 제출
gen.submit_read(0x1000, 4096, callback);
gen.run_until_complete();
```

### 2. Configuration 간소화

**Before (MQSim_CXL)**:
- XML 파일 (`ssdconfig.xml`)
- Text 파일 (`config.txt`)
- Workload 파일 (`workload.xml`)
- RapidXML dependency

**After (Traffic Generator)**:
```cpp
// C++ struct
CXL::TrafficGenerator::Config config;
config.dram_size = 64 * 1024 * 1024;
config.cache_policy = CachePolicy::CFLRU;
config.prefetcher = PrefetcherType::BEST_OFFSET;
// No file I/O, no XML parsing
```

### 3. Request 인터페이스

**Before (MQSim_CXL)**:
```cpp
// NVMe command 생성 필요
Submission_Queue_Entry* sqe = new Submission_Queue_Entry;
sqe->Opcode = NVME_READ_OPCODE;
sqe->Command_specific[0] = lba;
// ... 복잡한 설정

// PCIe message로 감싸기
PCIe_Message* msg = new PCIe_Message;
msg->Payload = sqe;
msg->Type = PCIe_Message_Type::WRITE_REQ;

// 전송
host_interface->Consume_pcie_message(msg);
```

**After (Traffic Generator)**:
```cpp
// 간단한 API
gen.submit_read(address, size, callback);
```

### 4. Simulation Control

**Before (MQSim_CXL)**:
```cpp
// 전역 singleton
Simulator->Start_simulation();  // Blocking call

// 이벤트 스케줄링
Simulator->Register_sim_event(time, object, params, type);
```

**After (Traffic Generator)**:
```cpp
// 명시적 제어
gen.tick(current_time);              // Step-by-step
gen.run_until(target_time);          // Run until time
gen.run_until_complete();            // Run until done
```

---

## 구현 전략

### PIMPL Pattern

Public API는 구현 세부사항을 숨기기 위해 PIMPL 패턴 사용:

```cpp
// include/cxl_traffic_gen.h (public header)
class TrafficGenerator {
public:
    // Public API
private:
    class Impl;  // Forward declaration
    std::unique_ptr<Impl> pimpl_;
};

// src/core/traffic_generator.cpp (implementation)
class TrafficGenerator::Impl {
    // Internal MQSim components
    CXL_Manager* cxl_manager_;
    SSD_Device* ssd_device_;
    // ...
};
```

### Callback 메커니즘

```cpp
// Request completion callback
using CompletionCallback = std::function<void(RequestID, uint64_t latency_ns)>;

// Internal: MQSim completion → Public callback
void TrafficGenerator::Impl::on_mqsim_completion(uint64_t lba, uint64_t latency) {
    auto it = callbacks_.find(lba);
    if (it != callbacks_.end()) {
        it->second(request_id, latency);  // Call user callback
        callbacks_.erase(it);
    }
}
```

---

## 빌드 시스템

### CMake 기반

```cmake
# Library target
add_library(cxl_traffic_gen
    ${CXL_SOURCES}
    ${SIM_SOURCES}
    ${SSD_SOURCES}
    ${FLASH_SOURCES}
)

# Public header
target_include_directories(cxl_traffic_gen
    PUBLIC include/
    PRIVATE src/
)

# Examples
add_executable(standalone_example examples/standalone_example.cpp)
target_link_libraries(standalone_example cxl_traffic_gen)
```

### 사용 방법

```bash
# Build as library
mkdir build && cd build
cmake ..
make -j$(nproc)

# Install
sudo make install

# Use in other projects
find_package(CXLTrafficGenerator REQUIRED)
target_link_libraries(your_target CXL::cxl_traffic_gen)
```

---

## Gem5 연동 방법

### 1. Traffic Generator를 Gem5 SimObject로 Wrapping

```cpp
// gem5/src/mem/cxl_flash/CXLFlashDevice.hh
class CXLFlashDevice : public SimObject {
    std::unique_ptr<CXL::TrafficGenerator> traffic_gen_;

    bool recvTimingReq(PacketPtr pkt) override;
    void tick();
};
```

### 2. Gem5 Packet → Traffic Generator Request

```cpp
bool CXLFlashDevice::recvTimingReq(PacketPtr pkt) {
    uint64_t addr = pkt->getAddr();
    uint32_t size = pkt->getSize();

    traffic_gen_->submit_read(addr, size,
        [this, pkt](uint64_t id, uint64_t latency_ns) {
            // Convert latency to Gem5 ticks
            Tick ticks = SimClock::Int::ns * latency_ns;
            mem_side_port.schedTimingResp(pkt, curTick() + ticks);
        });

    return true;
}
```

### 3. Event System 동기화

```cpp
void CXLFlashDevice::tick() {
    // Advance traffic generator to current Gem5 time
    uint64_t current_ns = curTick() / SimClock::Int::ns;
    traffic_gen_->tick(current_ns);

    // Schedule next tick
    schedule(tickEvent, curTick() + 1000);  // Every 1000 ticks
}
```

---

## 다음 단계

### 완료된 작업 ✅
- [x] 컴포넌트 분석 및 분류
- [x] 디렉토리 구조 설계
- [x] Public API 헤더 작성
- [x] 예제 코드 작성
- [x] README 문서 작성
- [x] CMake 빌드 시스템

### 남은 작업 ⏭️
- [ ] `traffic_generator.cpp` 구현 (PIMPL)
- [ ] 필요한 SSD/Flash 파일 복사 및 수정
- [ ] 빌드 테스트
- [ ] Standalone 예제 실행 검증
- [ ] Gem5 연동 테스트
- [ ] 성능 벤치마크

---

## 참고 자료

- **Original MQSim_CXL**: https://github.com/dgist-datalab/MQSim_CXL
- **Paper**: "Overcoming the Memory Wall with CXL-Enabled SSDs" (USENIX ATC'23)
- **Developer Guide**: `../DEVELOPER_GUIDE_KR.md`
- **Architecture Analysis**: `../TRAFFIC_GEN_ANALYSIS.md`

---

## 라이선스

MIT License (MQSim_CXL과 동일)
