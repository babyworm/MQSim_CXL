# CXL Traffic Generator Tests

이 디렉토리에는 CXL Traffic Generator의 테스트 코드와 CXL flit 생성 도구가 포함되어 있습니다.

## 구성 요소

### 1. CXL Flit Generator (`cxl_flit_generator`)

모든 CXL flit 형식을 생성하고 hex dump를 출력하는 도구입니다.

**지원하는 Flit 타입:**
- `mem_rd` - Memory Read Request (메모리 읽기 요청)
- `mem_rd_data` - Memory Read with Data (데이터 포함 메모리 읽기)
- `mem_wr` - Memory Write Request (메모리 쓰기 요청)
- `mem_wr_ptl` - Memory Write Partial (부분 메모리 쓰기)
- `mem_data` - Memory Data Response (메모리 데이터 응답)
- `mem_data_nxm` - Non-Existent Memory Response (존재하지 않는 메모리 응답)
- `cpl` - Completion (완료 신호, 데이터 없음)
- `cpl_data` - Completion with Data (데이터 포함 완료 신호)
- `snp_data` - Snoop Data (캐시 일관성 스눕 데이터)
- `snp_inv` - Snoop Invalidate (캐시 무효화 스눕)

**사용법:**
```bash
# 빌드
make

# 모든 flit 타입 출력
./cxl_flit_generator

# 특정 flit 타입만 출력
./cxl_flit_generator mem_rd
./cxl_flit_generator mem_wr

# 모든 타입을 명시적으로 출력
./cxl_flit_generator all

# 출력을 파일로 저장
./cxl_flit_generator mem_rd > mem_rd_dump.txt
```

### 2. Batch Dump Script (`dump_all_flit_types.sh`)

모든 CXL flit 타입에 대한 hex dump를 자동으로 생성하는 스크립트입니다.

**사용법:**
```bash
# 모든 flit 타입 dump 생성
./dump_all_flit_types.sh
```

**생성되는 파일:**
- `flit_dumps/mem_rd.txt` - MEM_RD flit dump
- `flit_dumps/mem_rd_data.txt` - MEM_RD_DATA flit dump
- `flit_dumps/mem_wr.txt` - MEM_WR flit dump
- `flit_dumps/mem_wr_ptl.txt` - MEM_WR_PTL flit dump
- `flit_dumps/mem_data.txt` - MEM_DATA flit dump
- `flit_dumps/mem_data_nxm.txt` - MEM_DATA_NXM flit dump
- `flit_dumps/cpl.txt` - CPL flit dump
- `flit_dumps/cpl_data.txt` - CPL_DATA flit dump
- `flit_dumps/snp_data.txt` - SNP_DATA flit dump
- `flit_dumps/snp_inv.txt` - SNP_INV flit dump
- `flit_dumps/all_flit_types.txt` - 모든 타입 통합 dump

**예제:**
```bash
# Batch dump 실행
./dump_all_flit_types.sh

# 특정 flit 타입 확인
cat flit_dumps/mem_rd.txt

# 모든 타입 한번에 보기
cat flit_dumps/all_flit_types.txt

# Hex 값만 추출
grep "0x" flit_dumps/mem_rd.txt
```

### 3. Traffic Pattern Tests (`test_cxl_traffic_patterns`)

7가지 일반적인 CXL 사용 패턴을 테스트하는 프로그램입니다.

**테스트 패턴:**
1. Sequential Read (Streaming) - 순차 읽기
2. Random Read (Database Lookup) - 무작위 읽기
3. Write-Back (Cache Eviction) - 캐시 플러시
4. Read-Modify-Write (Atomic Operation) - 원자적 RMW
5. Prefetch Requests (Next-N-Line) - 하드웨어 prefetch
6. Mixed Read/Write Workload - 혼합 워크로드
7. Burst Transfer (DMA-like) - 버스트 전송

**사용법:**
```bash
# 모든 테스트 실행
./test_cxl_traffic_patterns

# 출력을 파일로 저장
./test_cxl_traffic_patterns > patterns_output.txt
```

## CXL 2.0 256-Byte Flit 구조

```
+------------------+
| Header (16B)     |  Protocol ID, Opcode, Tag, Address, Length, etc.
+------------------+
| Data (240B)      |  Payload data
+------------------+
Total: 256 bytes
```

**헤더 필드:**
- `protocol_id` (1B): 0x0=CXL.io, 0x1=CXL.cache, 0x2=CXL.mem
- `opcode` (1B): Operation code (MEM_RD, MEM_WR, etc.)
- `tag` (2B): Transaction tag for matching requests/responses
- `address` (8B): Physical memory address
- `length` (2B): Data length in bytes
- `cache_id` (1B): Cache identifier
- `flags` (1B): Additional flags

## 빌드 및 실행

### Make를 사용한 빌드
```bash
# 모든 테스트 빌드
make

# 특정 테스트만 빌드
make cxl_flit_generator
make test_cxl_traffic_patterns

# 빌드 결과 정리
make clean

# 테스트 실행
make run
```

### CMake를 사용한 빌드
```bash
# 프로젝트 루트에서
mkdir build && cd build
cmake ..
make

# 테스트 실행
./tests/cxl_flit_generator
./tests/test_cxl_traffic_patterns
```

## 출력 예제

### Flit Header 출력
```
--- CXL Flit Header ---
  Protocol ID: 0x2 (CXL.mem)
  Opcode:      0x0 (MEM_RD)
  Tag:         0x100
  Address:     0x0000000001000000
  Length:      4096 bytes
  Cache ID:    0
  Flags:       0x0
```

### Hex Dump 출력
```
=== MEM_RD (Memory Read) (256 bytes) ===
  0000: 02 00 00 01 00 00 00 01 00 00 00 00 00 10 00 00
  0010: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  ...
```

## 파일 목록

- `cxl_flit_generator.cpp` - CXL flit 생성 도구
- `test_cxl_traffic_patterns.cpp` - 트래픽 패턴 테스트
- `dump_all_flit_types.sh` - Batch dump 스크립트
- `Makefile` - Make 빌드 설정
- `CMakeLists.txt` - CMake 빌드 설정
- `.gitignore` - Git 제외 파일 설정
- `cxl_flit_samples.txt` - 샘플 출력 (참고용)

## 참고 자료

- CXL 2.0 Specification
- `../include/cxl_flit.h` - CXL flit 구조 정의
- `../README.md` - 전체 프로젝트 README
