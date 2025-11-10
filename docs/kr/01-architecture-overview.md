# Chapter 1: 아키텍처 개요

[← 메인으로](../../DEVELOPER_GUIDE_KR.md) | [다음: CXL 구현 →](02-cxl-implementation.md)

---

## 목차
1. [프로젝트 전체 구조](#프로젝트-전체-구조)
2. [디렉토리 구조 분석](#디렉토리-구조-분석)
3. [핵심 컴포넌트 다이어그램](#핵심-컴포넌트-다이어그램)
4. [클래스 계층 구조](#클래스-계층-구조)
5. [데이터 경로 분석](#데이터-경로-분석)

---

## 프로젝트 전체 구조

### 개요
MQSim_CXL은 CXL 프로토콜을 지원하는 Flash Memory 디바이스의 **trace-driven simulator**입니다. Discrete Event Simulation (DES) 방식을 사용하여 nanosecond 단위의 정확한 타이밍 시뮬레이션을 제공합니다.

### 핵심 특징
- **Full Stack Simulation**: Host Interface → Cache → FTL → Flash Chip
- **CXL.mem 지원**: Device-side DRAM cache with prefetching
- **정확한 타이밍**: Flash read/program/erase latency 모델링
- **확장 가능**: 다양한 cache policy, prefetcher 알고리즘

---

## 디렉토리 구조 분석

```
MQSim_CXL/
├── src/                              # 소스 코드 루트
│   ├── cxl/                          # ⭐ CXL 구현 (20 files, ~6K LOC)
│   │   ├── Host_Interface_CXL.h/cpp  # CXL 호스트 인터페이스 (1,482 LOC)
│   │   ├── CXL_Manager              # 핵심 CXL 관리자
│   │   ├── DRAM_Subsystem.h/cpp     # DRAM 캐시 구현 (909 LOC)
│   │   ├── CXL_MSHR.h/cpp           # Miss Status Handling Register
│   │   ├── CXL_Config.h/cpp         # 설정 파서
│   │   ├── DRAM_Model.h/cpp         # DRAM 타이밍 모델
│   │   ├── Prefetching_Alg.h/cpp    # Prefetcher 알고리즘
│   │   ├── CFLRU.h/cpp              # Clock-FIFO-LRU 정책
│   │   └── lrfu_heap.h/cpp          # LRFU 정책 구현
│   │
│   ├── ssd/                          # SSD 시뮬레이션 (47 files)
│   │   ├── Host_Interface_Base.h    # 호스트 인터페이스 추상 클래스
│   │   ├── Host_Interface_NVMe.h    # NVMe 프로토콜
│   │   ├── FTL.h/cpp                # Flash Translation Layer (1,083 LOC)
│   │   ├── Address_Mapping_Unit_*   # LBA→PBA 매핑
│   │   ├── Flash_Block_Manager.*    # 블록 관리 및 상태 추적
│   │   ├── GC_and_WL_Unit_*         # Garbage Collection + Wear Leveling
│   │   ├── TSU_*.h/cpp              # Transaction Scheduling Unit
│   │   ├── Data_Cache_Manager_*     # 데이터 캐시 관리
│   │   ├── NVM_PHY_*.h/cpp          # Physical Layer (ONFI)
│   │   ├── ONFI_Channel_*.h/cpp     # 채널 컨트롤러
│   │   └── NVM_Transaction_*.h/cpp  # Flash 트랜잭션
│   │
│   ├── host/                         # 호스트 시스템 (11 files)
│   │   ├── IO_Flow_Base.h/cpp       # I/O 플로우 추상 클래스
│   │   ├── IO_Flow_Trace_Based.h    # Trace 기반 워크로드
│   │   ├── IO_Flow_Synthetic.h      # Synthetic 워크로드
│   │   ├── PCIe_Root_Complex.h      # PCIe 루트 컴플렉스
│   │   ├── PCIe_Switch.h            # PCIe 스위치
│   │   └── PCIe_Link.h              # PCIe 링크 (대역폭 모델링)
│   │
│   ├── sim/                          # ⭐ Discrete Event Engine (7 files)
│   │   ├── Engine.h/cpp             # 시뮬레이션 엔진 (singleton)
│   │   ├── EventTree.h/cpp          # 이벤트 우선순위 큐 (503 LOC)
│   │   ├── Sim_Object.h             # 시뮬레이션 객체 기본 클래스
│   │   ├── Sim_Event.h              # 이벤트 표현
│   │   └── Sim_Reporter.h           # 통계 수집
│   │
│   ├── exec/                         # 실행 및 설정 (7 files)
│   │   ├── SSD_Device.h/cpp         # SSD 디바이스 통합 (436 LOC)
│   │   ├── Host_System.h/cpp        # 호스트 시스템 통합
│   │   ├── Execution_Parameter_Set  # 실행 파라미터
│   │   ├── Device_Parameter_Set     # 디바이스 설정 (630 LOC)
│   │   └── IO_Flow_Parameter_Set    # 워크로드 설정 (479 LOC)
│   │
│   ├── nvm_chip/flash_memory/        # Flash 칩 모델 (8 files)
│   │   ├── Flash_Chip.h/cpp         # 칩 레벨 시뮬레이터
│   │   ├── Die.h/cpp                # Die 관리
│   │   ├── Plane.h/cpp              # Plane 연산
│   │   ├── Block.h/cpp              # 블록 상태
│   │   ├── Page.h                   # 페이지 상태
│   │   └── Physical_Page_Address.h  # 주소 변환
│   │
│   ├── utils/                        # 유틸리티
│   │   ├── rapidxml/                # XML 파서
│   │   ├── XMLWriter.h/cpp          # 결과 출력
│   │   └── RandomGenerator.h        # 랜덤 생성기
│   │
│   └── main.cpp                      # ⭐ 프로그램 진입점
│
├── config.txt                        # CXL 아키텍처 설정
├── ssdconfig.xml                     # SSD 디바이스 스펙
├── workload.xml                      # 워크로드 정의
├── Makefile                          # Linux 빌드
└── MQSim.sln                         # Windows (VS 2022)
```

### 주요 디렉토리별 역할

| 디렉토리 | 역할 | LOC | 주요 파일 |
|---------|------|-----|----------|
| **src/cxl/** | CXL 프로토콜 및 DRAM 캐시 구현 | ~6,000 | Host_Interface_CXL.cpp (1,482) |
| **src/ssd/** | SSD 시뮬레이션 (FTL, GC, 스케줄링) | ~8,000 | FTL.cpp, Address_Mapping_Unit_Page_Level.cpp |
| **src/sim/** | Discrete Event Engine | ~1,500 | EventTree.cpp (503) |
| **src/host/** | 호스트 I/O 생성 및 PCIe | ~2,000 | IO_Flow_Trace_Based.cpp |
| **src/nvm_chip/** | Flash 칩 하드웨어 모델 | ~1,500 | Flash_Chip.cpp |

---

## 핵심 컴포넌트 다이어그램

### 전체 시스템 아키텍처

```mermaid
graph TB
    subgraph "Host Layer"
        APP[Application/Trace]
        IOFLOW[IO_Flow_Base<br/>Trace/Synthetic]
        RC[PCIe_Root_Complex]
    end

    subgraph "SSD Device"
        subgraph "Host Interface Layer"
            HI_BASE[Host_Interface_Base]
            HI_CXL[Host_Interface_CXL]
            HI_NVME[Host_Interface_NVMe]
            HI_BASE -.-> HI_CXL
            HI_BASE -.-> HI_NVME
        end

        subgraph "CXL Manager Layer"
            CXLMGR[CXL_Manager]
            DRAM[DRAM_Subsystem<br/>Cache + Policies]
            MSHR[CXL_MSHR<br/>Hit-under-Miss]
            PREF[Prefetchers<br/>Tagged/BO/LEAP]
            DRAMMODEL[DRAM_Model<br/>Timing]
        end

        subgraph "Cache Layer"
            DC[Data_Cache_Manager]
        end

        subgraph "FTL Layer"
            FTL[NVM_Firmware / FTL]
            ADDR[Address_Mapping_Unit<br/>Page-Level/Hybrid]
            BM[Flash_Block_Manager<br/>Block Pool]
            GC[GC_and_WL_Unit<br/>GC + Wear-Leveling]
            TSU[TSU<br/>Transaction Scheduler]
        end

        subgraph "Physical Layer"
            PHY[NVM_PHY_ONFI]
            CH[ONFI_Channel<br/>×8 channels]
        end

        subgraph "Flash Chips"
            CHIP[Flash_Chip<br/>×8 chips per channel<br/>Die → Plane → Block → Page]
        end
    end

    subgraph "Simulation Engine"
        ENG[Engine<br/>Discrete Event Simulator]
        EVT[EventTree<br/>Priority Queue]
    end

    APP --> IOFLOW
    IOFLOW --> RC
    RC --> HI_CXL
    HI_CXL --> CXLMGR
    CXLMGR --> DRAM
    CXLMGR --> MSHR
    CXLMGR --> PREF
    CXLMGR --> DRAMMODEL
    DRAM -->|Cache Miss| FTL
    HI_CXL --> DC
    DC --> FTL
    FTL --> ADDR
    FTL --> BM
    FTL --> GC
    FTL --> TSU
    TSU --> PHY
    PHY --> CH
    CH --> CHIP

    ENG --> EVT
    ENG -.->|Schedule Events| HI_CXL
    ENG -.->|Schedule Events| FTL
    ENG -.->|Schedule Events| PHY

    style CXLMGR fill:#f9f,stroke:#333,stroke-width:4px
    style DRAM fill:#bbf,stroke:#333,stroke-width:2px
    style FTL fill:#bfb,stroke:#333,stroke-width:2px
    style ENG fill:#fbb,stroke:#333,stroke-width:2px
```

---

## 클래스 계층 구조

### 1. Host Interface 계층

```mermaid
classDiagram
    class Sim_Object {
        <<abstract>>
        +sim_object_id_type ID
        +Execute_simulator_event()*
    }

    class Host_Interface_Base {
        <<abstract>>
        +Data_Cache_Manager* cache
        +NVM_Firmware* firmware
        +Start_simulation()*
        +Validate_simulation_config()*
        +Execute_simulator_event()*
    }

    class Host_Interface_CXL {
        +CXL_Manager* cxl_man
        +CXL_DRAM_Model* cxl_dram
        +Consume_pcie_message()
        +Update_CXL_DRAM_state()
        +process_CXL_prefetch_requests()
    }

    class Host_Interface_NVMe {
        +Input_Stream_Manager_NVMe
        +Request_Fetch_Unit_NVMe
        +process_pcie_write_message()
        +process_pcie_read_message()
    }

    class Host_Interface_SATA {
        +NCQ support
        +SATA protocol
    }

    Sim_Object <|-- Host_Interface_Base
    Host_Interface_Base <|-- Host_Interface_CXL
    Host_Interface_Base <|-- Host_Interface_NVMe
    Host_Interface_Base <|-- Host_Interface_SATA
```

### 2. CXL Manager 및 서브시스템

```mermaid
classDiagram
    class CXL_Manager {
        +dram_subsystem* dram
        +cxl_mshr* mshr
        +boClass boPrefetcher
        +leapClass leapPrefetcher
        +process_requests()
        +request_serviced()
        +prefetch_decision_maker()
        +prefetch_feedback()
    }

    class dram_subsystem {
        +map~uint64_t, uint64_t~ dram_mapping
        +list~uint64_t~ freeCL
        +cxl_config cpara
        +isCacheHit()
        +process_cache_hit()
        +process_miss_data_ready_new()
        +get_cache_index()
    }

    class cxl_mshr {
        +int row[1024][65]
        +map~uint64_t, list~ lba_transaction_map
        +isDuplicate()
        +insertEntry()
        +removeEntry()
    }

    class CXL_DRAM_Model {
        +uint64_t tRCD, tCL, tRP
        +service_cxl_dram_access()
        +calculate_latency()
    }

    CXL_Manager --> dram_subsystem
    CXL_Manager --> cxl_mshr
    CXL_Manager --> CXL_DRAM_Model
```

### 3. FTL (Flash Translation Layer) 계층

```mermaid
classDiagram
    class NVM_Firmware {
        +Address_Mapping_Unit* address_mapping_unit
        +Flash_Block_Manager* block_manager
        +GC_and_WL_Unit* gc_and_wl_unit
        +TSU_Base* tsu
        +Perform_translation()
        +Start_servicing_writes()
        +Check_and_run_GC()
    }

    class Address_Mapping_Unit_Base {
        <<abstract>>
        +Allocate_address_for_preconditioning()*
        +Allocate_new_page_for_write()*
        +Get_physical_address()*
    }

    class Address_Mapping_Unit_Page_Level {
        +GlobalMappingTable
        +CachedMappingTable (CMT)
        +Translate_lpa_to_ppa()
        +Allocate_new_page_for_write()
    }

    class Flash_Block_Manager {
        +Block_Pool_Slot_Type** plane_manager
        +Get_a_free_block()
        +Invalidate_page_in_block()
        +Is_having_enough_free_pages()
    }

    class GC_and_WL_Unit_Page_Level {
        +Check_gc_required()
        +GC_Src_Block_Selection()
        +GC_Dst_Block_Selection()
        +Run_static_wearleveling()
    }

    class TSU_Base {
        <<abstract>>
        +Flash_Transaction_Queue** UserReadTRQueue
        +Flash_Transaction_Queue** UserWriteTRQueue
        +Flash_Transaction_Queue** GC_Read_TR_Queue
        +Submit_transaction()*
        +Schedule()*
    }

    NVM_Firmware --> Address_Mapping_Unit_Base
    NVM_Firmware --> Flash_Block_Manager
    NVM_Firmware --> GC_and_WL_Unit_Page_Level
    NVM_Firmware --> TSU_Base
    Address_Mapping_Unit_Base <|-- Address_Mapping_Unit_Page_Level
```

### 4. Discrete Event Simulation Engine

```mermaid
classDiagram
    class Engine {
        <<singleton>>
        -sim_time_type _sim_time
        -EventTree* _EventList
        -unordered_map~sim_object_id, Sim_Object*~ _ObjectList
        +Time()
        +Register_sim_event()
        +Start_simulation()
        +Stop_simulation()
    }

    class EventTree {
        -list~Sim_Event*~ timeline
        +Insert()
        +Get_next_event()
        +Remove()
    }

    class Sim_Event {
        +sim_time_type Fire_time
        +Sim_Object* Target_sim_object
        +void* Parameters
        +int Type
    }

    class Sim_Object {
        <<abstract>>
        +sim_object_id_type ID
        +Execute_simulator_event()*
    }

    Engine --> EventTree
    Engine --> Sim_Object
    EventTree --> Sim_Event
    Sim_Event --> Sim_Object
```

---

## 데이터 경로 분석

### Read Request 전체 경로

```mermaid
sequenceDiagram
    autonumber
    participant Trace as Trace File
    participant IOFlow as IO_Flow_Trace_Based
    participant PCIe as PCIe_Root_Complex
    participant HI as Host_Interface_CXL
    participant CXLM as CXL_Manager
    participant DRAM as DRAM_Subsystem
    participant MSHR as CXL_MSHR
    participant FTL as NVM_Firmware
    participant TSU as TSU_OutofOrder
    participant PHY as NVM_PHY_ONFI
    participant Chip as Flash_Chip

    Trace->>IOFlow: Load request (time, LBA, size)
    IOFlow->>PCIe: Create NVMe command
    PCIe->>HI: PCIe_Message (READ)
    HI->>CXLM: process_requests(address)

    CXLM->>DRAM: isCacheHit(LBA)?

    alt Cache Hit
        DRAM-->>CXLM: true
        CXLM->>DRAM: process_cache_hit()
        DRAM-->>HI: Return cached data
        HI->>PCIe: Completion
        PCIe->>IOFlow: Update statistics
    else Cache Miss
        DRAM-->>CXLM: false
        CXLM->>MSHR: isDuplicate(LBA)?

        alt Already in MSHR
            MSHR-->>CXLM: true (hit-under-miss)
            Note over CXLM: Wait for ongoing request
        else New Miss
            MSHR-->>CXLM: false
            MSHR->>MSHR: insertEntry(LBA)
            CXLM->>FTL: Forward to Flash Backend
            FTL->>FTL: Translate LPA→PPA
            FTL->>TSU: Submit_transaction(READ)
            TSU->>PHY: Execute flash command
            PHY->>Chip: Issue READ command

            Note over Chip: Flash Read Latency<br/>(3~75 μs)

            Chip-->>PHY: Data ready
            PHY-->>TSU: Transaction complete
            TSU-->>FTL: Notify completion
            FTL-->>CXLM: Data arrival
            CXLM->>MSHR: removeEntry(LBA)
            CXLM->>DRAM: process_miss_data_ready()
            DRAM->>DRAM: Fill cache (evict if needed)
            CXLM->>HI: Return data
            HI->>PCIe: Completion
            PCIe->>IOFlow: Update statistics
        end
    end

    CXLM->>CXLM: prefetch_decision_maker()
    CXLM->>HI: process_CXL_prefetch_requests()
```

### Cache Miss 시 Eviction 경로

```mermaid
graph TB
    START[Cache Miss - Need Eviction]

    START --> CHECK_POLICY{Cache Policy?}

    CHECK_POLICY -->|FIFO| FIFO[Get oldest cache line]
    CHECK_POLICY -->|LRU| LRU[Get LRU cache line]
    CHECK_POLICY -->|CFLRU| CFLRU[Clock + FIFO + LRU]
    CHECK_POLICY -->|LRU-2| LRU2[Active/Inactive lists]
    CHECK_POLICY -->|LRFU| LRFU[Heap-based CRF]

    FIFO --> CHECK_DIRTY
    LRU --> CHECK_DIRTY
    CFLRU --> CHECK_DIRTY
    LRU2 --> CHECK_DIRTY
    LRFU --> CHECK_DIRTY

    CHECK_DIRTY{Dirty bit set?}

    CHECK_DIRTY -->|Yes| FLUSH[Flush to Flash<br/>Create WRITE transaction]
    CHECK_DIRTY -->|No| EVICT[Evict cache line]

    FLUSH --> EVICT
    EVICT --> FILL[Fill with new data]
    FILL --> UPDATE[Update mapping]
    UPDATE --> DONE[Done]

    style START fill:#f9f
    style CHECK_DIRTY fill:#ff9
    style FLUSH fill:#f99
    style DONE fill:#9f9
```

### Write Request 처리

```mermaid
graph LR
    subgraph "Write Path"
        W1[Host Write] --> W2[CXL_Manager]
        W2 --> W3{Has DRAM Cache?}

        W3 -->|Yes| W4[Write to DRAM]
        W3 -->|No| W5[Direct to Flash]

        W4 --> W6[Mark Dirty]
        W6 --> W7{Eviction?}
        W7 -->|Yes| W8[Flush to Flash]
        W7 -->|No| W9[Complete]

        W5 --> W10[FTL Processing]
        W8 --> W10

        W10 --> W11[Address Mapping]
        W11 --> W12[Allocate New Page]
        W12 --> W13[TSU Schedule]
        W13 --> W14[Flash Program]

        W14 --> W15[Complete]
        W9 --> DONE[Done]
        W15 --> DONE
    end

    style W4 fill:#9cf
    style W8 fill:#f99
    style W14 fill:#fc9
```

---

## Flash 하드웨어 계층 구조

### Flash Chip 내부 구조

```mermaid
graph TB
    subgraph "Flash Chip"
        CHIP[Flash_Chip<br/>per Channel]

        subgraph "Die Level"
            DIE1[Die 0]
            DIE2[Die 1]
            DIEN[Die N-1]
        end

        subgraph "Plane Level - Die 0"
            PLANE1[Plane 0]
            PLANE2[Plane 1]
            PLANE3[Plane 2]
            PLANE4[Plane 3]
        end

        subgraph "Block Level - Plane 0"
            BLOCK1[Block 0<br/>Status: FREE]
            BLOCK2[Block 1<br/>Status: ACTIVE]
            BLOCKN[Block 511<br/>Status: GC_SOURCE]
        end

        subgraph "Page Level - Block 1"
            PAGE1[Page 0<br/>VALID<br/>LPA: 1234]
            PAGE2[Page 1<br/>INVALID]
            PAGE3[Page 2<br/>VALID<br/>LPA: 5678]
            PAGEN[Page 511<br/>FREE]
        end

        subgraph "Physical Address"
            ADDR[Physical_Page_Address<br/>Channel: 0<br/>Chip: 3<br/>Die: 0<br/>Plane: 1<br/>Block: 42<br/>Page: 128]
        end
    end

    CHIP --> DIE1
    CHIP --> DIE2
    CHIP --> DIEN

    DIE1 --> PLANE1
    DIE1 --> PLANE2
    DIE1 --> PLANE3
    DIE1 --> PLANE4

    PLANE1 --> BLOCK1
    PLANE1 --> BLOCK2
    PLANE1 --> BLOCKN

    BLOCK2 --> PAGE1
    BLOCK2 --> PAGE2
    BLOCK2 --> PAGE3
    BLOCK2 --> PAGEN

    PAGE1 -.-> ADDR

    style CHIP fill:#f9f
    style DIE1 fill:#9cf
    style PLANE1 fill:#9fc
    style BLOCK2 fill:#fc9
    style PAGE1 fill:#cf9
```

### Flash Command Execution 타이밍

```mermaid
gantt
    title Flash Command Execution Timeline
    dateFormat X
    axisFormat %L

    section Channel 0
    CMD Transfer       :0, 100
    Address Transfer   :100, 200
    Data Transfer (32KB) :crit, 200, 2400

    section Flash Chip
    tRCD (Row Access)  :300, 13
    Data Read (Page)   :crit, 313, 3000
    tCL (Column)       :3313, 13
    Data Out           :3326, 2000

    section Completion
    Return to Host     :done, 5326, 100
```

---

## Configuration 파일 구조

### config.txt (CXL 아키텍처)

```
┌─────────────────────────────────────────┐
│ CXL Configuration (config.txt)          │
├─────────────────────────────────────────┤
│ DRAM_mode           0                   │ ← 0: CXL-flash, 1: DRAM only
│ Has_cache           1                   │ ← Enable DRAM cache
│ DRAM_size           67108864            │ ← 64 MB (bytes)
│ Mix_mode            1                   │ ← Mix demand + prefetch
│ Cache_portion_percentage  100           │ ← Cache size ratio
│ Has_mshr            1                   │ ← Enable MSHR
│ Cache_placement     16                  │ ← 16-way set associative
│ Cache_policy        CFLRU               │ ← FIFO/LRU/CFLRU/...
│ Prefetcher          Best-offset         │ ← No/Tagged/BO/LEAP
│ Total_number_of_requests  20000000      │ ← Workload size
└─────────────────────────────────────────┘
```

### ssdconfig.xml (Flash Device)

```
┌──────────────────────────────────────────────────┐
│ SSD Configuration (ssdconfig.xml)                │
├──────────────────────────────────────────────────┤
│ Flash Hierarchy:                                 │
│   - Channels: 8                                  │
│   - Chips per Channel: 8                         │
│   - Dies per Chip: 1                             │
│   - Planes per Die: 4                            │
│   - Blocks per Plane: 512                        │
│   - Pages per Block: 512                         │
│   - Page Capacity: 16 KB                         │
│                                                  │
│ Flash Technology: SLC/MLC/TLC                    │
│                                                  │
│ Latencies (nanoseconds):                         │
│   - Page Read: 3,000 (SLC) ~ 75,000 (TLC MSB)   │
│   - Page Program: 100,000 ~ 1,800,000           │
│   - Block Erase: 1,000,000 ~ 10,000,000         │
│                                                  │
│ FTL Configuration:                               │
│   - Address Mapping: PAGE_LEVEL                  │
│   - GC Policy: GREEDY/RGA/RANDOM                 │
│   - Overprovisioning: 12.7%                      │
│   - Transaction Scheduling: OUT_OF_ORDER         │
└──────────────────────────────────────────────────┘
```

---

## 통계 및 출력

### overall.txt 출력 예시

```
=== CXL-Flash Statistics ===
Total Accesses:           20,000,000
Cache Hit Count:          18,500,000  (92.5%)
Cache Miss Count:          1,500,000  (7.5%)
Prefetch Hit Count:          800,000  (4.0%)
Flash Read Count:          1,200,000
Flash Write Count (Flush):   400,000
Hit-under-Miss Count:         50,000

=== Prefetcher Performance ===
Coverage:                      53.3%
Accuracy:                      66.7%
Lateness:                       0.8%
Pollution:                     12.0%

=== Flash Backend ===
Total Flash Accesses:        1,600,000
Average Queue Depth:              12.4
GC Invocations:                  2,340
Pages Moved by GC:             120,000

=== Lifetime Estimation ===
Device Lifetime:              2.3 years (at current workload)
```

---

## 주요 인터페이스 정리

### Gem5 연동 시 주요 진입점

| 클래스/함수 | 역할 | 위치 |
|-----------|------|------|
| `Host_Interface_CXL::Consume_pcie_message()` | PCIe 메시지 수신 | Host_Interface_CXL.h:197 |
| `CXL_Manager::process_requests()` | CXL 요청 처리 | Host_Interface_CXL.h:39 |
| `Engine::Register_sim_event()` | 이벤트 스케줄링 | Engine.h:29 |
| `Engine::Time()` | 현재 시뮬레이션 시간 | Engine.h:28 |
| `SSD_Device::Execute_simulator_event()` | 이벤트 실행 | SSD_Device.h:52 |

---

## 다음 챕터

이제 CXL 구현의 상세한 내용을 살펴보겠습니다.

[다음: Chapter 2 - CXL 구현 상세 →](02-cxl-implementation.md)

---

[← 메인으로](../../DEVELOPER_GUIDE_KR.md)
