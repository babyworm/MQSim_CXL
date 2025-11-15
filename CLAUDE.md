# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MQSim CXL** is a trace-driven simulator for CXL-enabled flash memory devices. It simulates a CXL-flash device that combines device-side DRAM cache with a flash backend, enabling main memory expansion via CXL (Compute Express Link). The simulator is built on top of MQSim-E and includes comprehensive models for cache policies, prefetchers, MSHR (Miss Status Holding Registers), FTL (Flash Translation Layer), garbage collection, and wear-leveling.

This is a research project from S4 Group (Syracuse University), DGIST DataLab, DATOS Lab (Soongsil University), and FADU, supporting the paper "Overcoming the Memory Wall with CXL-Enabled SSDs" (USENIX ATC'23).

## Build System

### Linux Build (Primary Development Platform)

```bash
# Build the simulator
make

# Build CXL Traffic Generators
make TG

# Clean all build artifacts
make clean
```

**Build output:**
- Executable: `./MQSim` (in repository root)
- Build artifacts: `build/` directory

**Build flags:**
- Compiler: `g++` with `-std=c++11 -O3 -g`
- Modules: `exec`, `host`, `nvm_chip`, `nvm_chip/flash_memory`, `sim`, `ssd`, `utils`, `cxl`

### Windows Build (Original Development Platform)

Open `MQSim.sln` in Visual Studio 2022:
1. Set Solution Configuration to **Release** (not Debug)
2. Build solution
3. Run `MQSim.exe` with arguments: `-i ssdconfig.xml -w workload.xml`

## Running the Simulator

### Command Line Execution

```bash
./MQSim -i ssdconfig.xml -w workload.xml
```

### Required Setup

1. **Create directories:**
   ```bash
   mkdir -p traces Results
   ```

2. **Configuration files** (in repository root):
   - `config.txt` - CXL-flash architecture configuration
   - `ssdconfig.xml` - Flash backend and device parameters
   - `workload.xml` - Trace file and workload definitions

3. **Input traces:**
   - Place `*.trace` files in `traces/` directory
   - Update `File_Path` in `workload.xml`
   - Set `Total_number_of_requests` in `config.txt`

4. **Output:**
   - Results written to `Results/` directory
   - `overall.txt` - Cache hits, flash reads/writes, prefetcher metrics
   - `latency_result.txt` - Per-request latency in nanoseconds
   - `repeated_access.txt` - Repeated access tracking (when MSHR disabled)

## Architecture Overview

### High-Level Component Hierarchy

```
SSD_Device
├── Host_Interface_Base (CXL, NVMe, or SATA)
│   └── CXL_Manager (manages CXL-specific logic)
│       ├── DRAM_Subsystem (device cache)
│       │   ├── Cache policies: FIFO, LRU, LRFU, CFLRU
│       │   └── DRAM_Model (timing model)
│       ├── CXL_MSHR (hit-under-miss support)
│       └── Prefetchers (Tagged, Best-Offset, LEAP, Feedback)
├── Data_Cache_Manager (host-side data cache)
├── NVM_Firmware (FTL)
│   ├── Address_Mapping_Unit (logical-to-physical mapping)
│   ├── Flash_Block_Manager (block allocation, wear tracking)
│   ├── GC_and_WL_Unit (garbage collection & wear-leveling)
│   └── TSU (Transaction Scheduling Unit)
├── NVM_PHY (physical layer)
└── NVM_Channels → Flash_Chips → Dies → Planes → Blocks → Pages
```

### Key Design Patterns

1. **Discrete Event Simulation:**
   - Central `Engine` (singleton) manages event queue via `EventTree`
   - All components inherit from `Sim_Object`
   - Events scheduled with nanosecond-precision timestamps
   - Main simulation loop in `Engine::Start_simulation()`

2. **CXL Request Flow:**
   ```
   Host → CXL_Manager::process_requests()
       → DRAM_Subsystem::isCacheHit()
       → [HIT]  → Return from cache (~100ns DRAM latency)
       → [MISS] → Check MSHR (hit-under-miss)
                → Forward to Flash Backend (FTL)
                → Flash read (3-75 μs depending on SLC/MLC/TLC)
                → Fill cache on completion
                → Prefetch decision
   ```

3. **Two-Level Caching:**
   - **Device-side DRAM cache** (CXL_Manager): Configured via `config.txt`
   - **Host-side data cache** (Data_Cache_Manager): Configured via `ssdconfig.xml` (legacy SSD cache)
   - For CXL simulation, typically set `DRAM_mode=0` and `Has_cache=1` in `config.txt`

## Critical Configuration Parameters

### config.txt (CXL-Flash Architecture)

```
DRAM_mode                    # 0=CXL-flash, 1=DRAM-only baseline
Has_cache                    # 0=no device cache, 1=enable DRAM cache
DRAM_size                    # Device cache size in bytes (e.g., 67108864 = 64MB)
Has_mshr                     # 0=no MSHR, 1=enable hit-under-miss
Cache_placement              # Set associativity (1 to DRAM_size/4096)
Cache_policy                 # Random, FIFO, LRU, CFLRU
Prefetcher                   # No, Tagged, Best-offset, Leap, Feedback_direct
Total_number_of_requests     # Number of requests in trace file
```

### ssdconfig.xml (Flash Backend)

**Critical flash parameters:**
- `Flash_Channel_Count` × `Chip_No_Per_Channel` = Flash parallelism
- `Flash_Technology`: SLC / MLC / TLC
- `Page_Read_Latency_LSB/CSB/MSB`: Read latencies in nanoseconds
- `Page_Program_Latency_LSB/CSB/MSB`: Write latencies in nanoseconds
- `Block_Erase_Latency`: Erase latency in nanoseconds
- `Overprovisioning_Ratio`: Reserved capacity (typically 0.127 = 12.7%)
- `GC_Exec_Threshold`: Start GC when free pages drop below threshold
- `GC_Block_Selection_Policy`: GREEDY, RGA, RANDOM, FIFO

**FTL configuration:**
- `Address_Mapping`: PAGE_LEVEL or HYBRID
- `Ideal_Mapping_Table`: true = infinite CMT (no mapping table reads)
- `Plane_Allocation_Scheme`: Resource allocation scheme (e.g., PCWD, CWDP)
- `Transaction_Scheduling_Policy`: OUT_OF_ORDER (default)

### workload.xml (Trace Configuration)

```xml
<File_Path>traces/synthetic_stride_p.trace</File_Path>
<Channel_IDs>0,1,2,3,4,5,6,7</Channel_IDs>
<Chip_IDs>0,1,2,3,4,5,6,7</Chip_IDs>
<Initial_Occupancy_Percentage>30</Initial_Occupancy_Percentage>
<Percentage_To_Be_Executed>100</Percentage_To_Be_Executed>
```

## Code Organization

### Source Structure (`src/`)

- **cxl/** - CXL-specific components
  - `Host_Interface_CXL.h/cpp` - Main CXL interface, integrates DRAM cache + MSHR + prefetchers
  - `CXL_Manager` - Request processing, cache hit/miss decisions
  - `DRAM_Subsystem.h/cpp` - Device DRAM cache with replacement policies
  - `CXL_MSHR.h/cpp` - Miss Status Holding Register for hit-under-miss
  - `Prefetching_Alg.h/cpp` - Prefetcher implementations (Best-Offset, LEAP, etc.)
  - `CFLRU.h/cpp` - Clean-First LRU cache policy
  - `OutputLog.h/cpp` - Statistics logging

- **ssd/** - SSD backend (FTL, caching, scheduling)
  - `FTL.h/cpp` - Flash Translation Layer coordinator
  - `Address_Mapping_Unit_*.h/cpp` - Logical-to-physical address mapping
  - `Flash_Block_Manager.h/cpp` - Block allocation and wear tracking
  - `GC_and_WL_Unit_*.h/cpp` - Garbage collection and wear-leveling
  - `TSU_*.h/cpp` - Transaction Scheduling Unit
  - `Host_Interface_Base.h/cpp` - Base for all host interfaces
  - `Data_Cache_Manager_*.h/cpp` - Legacy SSD data cache (distinct from CXL DRAM cache)

- **sim/** - Discrete event simulation engine
  - `Engine.h/cpp` - Central event scheduler (singleton)
  - `EventTree.h/cpp` - Event priority queue
  - `Sim_Object.h/cpp` - Base class for all simulated components
  - `Sim_Event.h` - Event structure

- **nvm_chip/** - Flash chip models
  - `flash_memory/` - Physical flash structure (Chip → Die → Plane → Block → Page)

- **exec/** - Execution and parameter management
  - `SSD_Device.h/cpp` - Top-level device instantiation
  - `Host_System.h/cpp` - Host-side simulation
  - `*_Parameter_Set.h` - Configuration parsing

- **host/** - Host I/O generation
  - `IO_Flow_Trace_Based.h/cpp` - Replay traces from files
  - `IO_Flow_Synthetic.h/cpp` - Generate synthetic workloads
  - `PCIe_*.h/cpp` - PCIe interconnect models

- **utils/** - Utilities (XML parsing, random generation, statistics)

### CXL Traffic Generator (Standalone Library)

Located in `cxl_traffic_gen/`:
- **Purpose:** Extracted library for Gem5 integration
- **Build:** `cd cxl_traffic_gen/tests && make`
- **API:** Simplified C++ interface for submitting read/write requests
- **See:** `cxl_traffic_gen/README.md` for standalone usage

## Modifying Prefetcher Parameters

Prefetcher parameters are **hardcoded** in `src/cxl/Host_Interface_CXL.h`:

```cpp
uint16_t prefetchK{4};              // Next-N-line degree (line 65)
uint16_t prefetch_timing_offset{16}; // Prefetch offset (line 66)
```

**To modify:** Edit these values and **recompile** the entire project.

## Common Development Workflows

### Adding a New Cache Policy

1. **Implement policy** in `src/cxl/DRAM_Subsystem.cpp`:
   - Add case to `dram_subsystem::evict_policy()` switch statement
   - Implement eviction logic using existing data structures

2. **Update config parsing** in `src/cxl/CXL_Config.cpp`:
   - Add policy name to `Cache_policy` enum
   - Update string-to-enum conversion

3. **Recompile:**
   ```bash
   make clean && make
   ```

### Adding a New Prefetcher

1. **Implement algorithm** in `src/cxl/Prefetching_Alg.h/cpp`
   - Create new class (see `boClass`, `leapClass` as examples)

2. **Integrate into CXL_Manager** in `src/cxl/Host_Interface_CXL.cpp`:
   - Instantiate in `CXL_Manager` constructor
   - Call from `prefetch_decision_maker()`

3. **Add to config** in `src/cxl/CXL_Config.cpp`

### Modifying Flash Timing

Edit `ssdconfig.xml`:
```xml
<Page_Read_Latency_LSB>3000</Page_Read_Latency_LSB>      <!-- nanoseconds -->
<Page_Program_Latency_LSB>100000</Page_Program_Latency_LSB>
<Block_Erase_Latency>1000000</Block_Erase_Latency>
```

For ULL (Ultra-Low Latency) flash:
```xml
<Flash_Technology>SLC</Flash_Technology>
<Page_Read_Latency_LSB>3000</Page_Read_Latency_LSB>
<Page_Program_Latency_LSB>100000</Page_Program_Latency_LSB>
<Block_Erase_Latency>1000000</Block_Erase_Latency>
```

### Running Experiments from the Paper

Example configurations are in `examples/`:
- `examples/synthetic workloads/` - Configurations for synthetic benchmarks
- `examples/real world workloads/` - Configurations for real-world traces

**Trace files:** Published at https://doi.org/10.5281/zenodo.7916219

**Workflow:**
1. Copy example configs to root: `cp examples/synthetic\ workloads/* .`
2. Download trace files to `traces/`
3. Modify `config.txt` parameters as needed (see README.md for paper-specific settings)
4. Run: `./MQSim -i ssdconfig.xml -w workload.xml`
5. Analyze results in `Results/overall.txt` and `Results/latency_result.txt`

## Simulation Performance

- **Synthetic workloads:** 5 minutes - 2 hours per experiment
- **Real-world workloads:** 30 minutes - 2 hours per experiment
- **Memory requirements:** Recommend ≥32GB DRAM for stable simulation
- **Output file sizes:** Can be 10s of GB for real-world traces

## Important Notes

1. **Windows vs Linux:**
   - Originally developed on Windows (Visual Studio 2022)
   - Linux support added via Makefile (PR #2)
   - Both platforms supported; Linux recommended for automation

2. **Preconditioning:**
   - Currently disabled (`Enabled_Preconditioning>false</Enabled_Preconditioning>`)
   - Preconditioning feature not yet updated for CXL mode

3. **Two Configuration Systems:**
   - `config.txt` - **CXL-specific** (cache, MSHR, prefetcher)
   - `ssdconfig.xml` / `workload.xml` - **MQSim legacy** (flash backend, FTL, traces)
   - Both files required for execution

4. **Resource Partitioning:**
   - `Channel_IDs`, `Chip_IDs`, `Die_IDs`, `Plane_IDs` in `workload.xml`
   - For no partitioning, allocate all resources to workload (e.g., all 8 channels)

5. **DRAM vs CXL Mode:**
   - `DRAM_mode=1` in `config.txt` → Pure DRAM baseline (no flash)
   - `DRAM_mode=0` → CXL-flash simulation

## Related Repositories

- **Trace Generator:** https://github.com/dgist-datalab/trace_generator.git
- **Trace Translator:** https://github.com/spypaul/trace_translation.git
- **MQSim CXL Linux:** https://github.com/spypaul/MQSim_CXL_Linux
- **Published Traces:** https://doi.org/10.5281/zenodo.7916219
