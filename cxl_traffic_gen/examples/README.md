# CXL Traffic Generator 예제

이 디렉토리는 CXL Traffic Generator의 사용 방법을 보여주는 예제 프로그램들을 포함합니다.

## 📁 예제 목록

### 1. `standalone_example.cpp`
독립 실행형 예제로, CXL Traffic Generator의 기본 사용법을 보여줍니다.

**주요 기능:**
- TrafficGenerator 설정 및 초기화
- 순차 읽기 요청 제출 (Sequential Read Pattern)
- 무작위 읽기 요청 제출 (Random Read Pattern)
- 시뮬레이션 실행 및 완료 대기
- 통계 정보 출력 (캐시 적중률, 평균 레이턴시 등)

**학습 내용:**
- CXL DRAM 캐시 설정 (크기, 정책, 프리페처)
- Flash 백엔드 설정 (채널, 칩, 레이턴시)
- 읽기 요청 제출 및 콜백 처리
- 시뮬레이션 실행 모드
- 성능 통계 수집 및 분석

### 2. `gem5_integration_example.cpp`
Gem5 시뮬레이터와의 통합 방법을 보여주는 의사 코드(pseudo-code) 예제입니다.

**주요 기능:**
- Gem5 SimObject로서의 CXL 디바이스 구현
- Gem5 메모리 포트 인터페이스 연동
- 타이밍 요청/응답 처리
- Gem5 통계(Statistics) 등록

**참고:**
- 이 예제는 실제 Gem5 환경이 필요한 의사 코드입니다
- Gem5 통합 패턴을 이해하기 위한 참고 자료입니다

## 🔨 빌드 방법

### 전제 조건
- C++14 이상
- CMake 3.10 이상
- Make 또는 Ninja

### 빌드 단계

```bash
# 프로젝트 루트 디렉토리에서
cd cxl_traffic_gen

# 빌드 디렉토리 생성
mkdir -p build
cd build

# CMake 설정
cmake ..

# 컴파일 (멀티코어 빌드)
make -j$(nproc)
```

빌드가 완료되면 다음 실행 파일들이 생성됩니다:
- `build/standalone_example`
- `build/gem5_integration_example`

## 🚀 실행 방법

### Standalone Example 실행

```bash
# build 디렉토리에서
./standalone_example
```

**예상 출력:**
```
=== CXL Traffic Generator - Standalone Example ===

Configuration:
  DRAM Size: 64 MB
  Cache Policy: CFLRU
  Prefetcher: Best-offset
  Flash Channels: 8
  Chips/Channel: 8

Submitting read requests...

Running simulation...
  Request 1234 (addr=0x0) completed in 3250 ns
  Request 1244 (addr=0x9000) completed in 120 ns
  ...
All requests completed successfully!

=== Statistics ===

Key Metrics:
  Total Requests: 100
  Cache Hit Rate: 92.00%
  Prefetch Accuracy: 85.00%
  Avg Latency: 320 ns (0.32 μs)
  Flash Reads: 8
  Flash Writes: 0

=== Testing Random Access Pattern ===
Submitting 50 random read requests...
  Cache Hit Rate (Random): 15.00%
  Avg Latency (Random): 2550 ns

Done!
```

### Gem5 Integration Example 실행

```bash
# build 디렉토리에서
./gem5_integration_example
```

**참고:** 이 예제는 테스트 목적의 의사 코드이며, 실제 Gem5 환경 없이 실행됩니다.

## 📊 예제별 상세 설명

### Standalone Example 분석

#### 1. 설정 단계
```cpp
TrafficGenerator::Config config;

// CXL DRAM 캐시 설정
config.dram_size = 64 * 1024 * 1024;  // 64 MB
config.cache_policy = TrafficGenerator::Config::CachePolicy::CFLRU;
config.prefetcher = TrafficGenerator::Config::PrefetcherType::BEST_OFFSET;
config.has_mshr = true;
config.set_associativity = 16;

// Flash 백엔드 설정
config.num_channels = 8;
config.chips_per_channel = 8;
config.flash_tech = TrafficGenerator::Config::FlashTechnology::SLC;
```

#### 2. 요청 제출
```cpp
// 순차 읽기 (프리페칭 효과 확인)
for (int i = 0; i < 100; i++) {
    uint64_t address = i * 4096;  // 4 KB 정렬
    auto req_id = gen.submit_read(address, 4096, callback);
}
```

#### 3. 시뮬레이션 실행
```cpp
// 모든 요청이 완료될 때까지 실행 (최대 10초)
bool all_completed = gen.run_until_complete(10'000'000'000);
```

#### 4. 통계 분석
```cpp
auto stats = gen.get_statistics();
std::cout << "Cache Hit Rate: " << (stats.hit_rate * 100.0) << "%\n";
std::cout << "Avg Latency: " << stats.avg_latency_ns << " ns\n";
```

### 성능 특성

| 접근 패턴 | 캐시 적중률 | 평균 레이턴시 | 설명 |
|---------|----------|-----------|------|
| **순차 읽기** | 92% | ~320 ns | 프리페칭 효과로 높은 적중률 |
| **무작위 읽기** | 15% | ~2550 ns | 프리페칭 효과 미미 |
| **캐시 적중** | - | ~100 ns | DRAM 접근 레이턴시 |
| **캐시 미스** | - | ~3 μs | SLC Flash 읽기 레이턴시 |

## 🔧 실험 및 튜닝

### 캐시 정책 비교
다른 캐시 정책을 시험하려면 `standalone_example.cpp`를 수정:

```cpp
// FIFO
config.cache_policy = TrafficGenerator::Config::CachePolicy::FIFO;

// LRU
config.cache_policy = TrafficGenerator::Config::CachePolicy::LRU;

// CFLRU (권장)
config.cache_policy = TrafficGenerator::Config::CachePolicy::CFLRU;
```

### 프리페처 알고리즘 비교
```cpp
// 프리페칭 없음
config.prefetcher = TrafficGenerator::Config::PrefetcherType::NONE;

// Tagged (Next-N-Line)
config.prefetcher = TrafficGenerator::Config::PrefetcherType::TAGGED;

// Best-Offset (권장)
config.prefetcher = TrafficGenerator::Config::PrefetcherType::BEST_OFFSET;

// LEAP
config.prefetcher = TrafficGenerator::Config::PrefetcherType::LEAP;
```

### DRAM 캐시 크기 조정
```cpp
// 32 MB
config.dram_size = 32 * 1024 * 1024;

// 64 MB (기본값)
config.dram_size = 64 * 1024 * 1024;

// 128 MB
config.dram_size = 128 * 1024 * 1024;
```

### Flash 기술 변경
```cpp
// SLC (빠름, 비쌈)
config.flash_tech = TrafficGenerator::Config::FlashTechnology::SLC;
config.page_read_latency_ns = 3000;      // 3 μs

// MLC (중간)
config.flash_tech = TrafficGenerator::Config::FlashTechnology::MLC;
config.page_read_latency_ns = 25000;     // 25 μs

// TLC (느림, 저렴)
config.flash_tech = TrafficGenerator::Config::FlashTechnology::TLC;
config.page_read_latency_ns = 75000;     // 75 μs
```

## 📈 사용 사례

### 1. 캐시 정책 연구
다양한 워크로드에서 캐시 교체 정책의 성능을 비교합니다.

### 2. 프리페칭 효과 분석
순차 vs 무작위 접근 패턴에서 프리페칭의 효과를 측정합니다.

### 3. CXL 메모리 확장 시뮬레이션
CXL을 통한 메모리 확장 시나리오를 시뮬레이션합니다.

### 4. Gem5 통합
전체 시스템 시뮬레이션에서 CXL 디바이스를 모델링합니다.

## 🔍 디버깅 및 로깅

상세 로깅을 활성화하려면:

```cpp
config.enable_logging = true;
config.verbose = true;
```

이렇게 하면 다음 정보가 출력됩니다:
- 캐시 적중/미스 이벤트
- Flash 읽기/쓰기 동작
- 프리페칭 결정
- MSHR 상태

## 📚 추가 자료

- **메인 README**: `../README.md` - 전체 API 문서 및 아키텍처 설명
- **추출 가이드**: `../EXTRACTION_GUIDE.md` - MQSim_CXL에서의 추출 과정
- **소스 코드**: `../src/` - 구현 세부사항
- **헤더 파일**: `../include/` - 공개 API 정의

## ❓ FAQ

**Q: 예제를 수정하려면 어떻게 하나요?**
A: 소스 파일(`.cpp`)을 수정한 후 `build/` 디렉토리에서 `make`를 다시 실행하세요.

**Q: 다른 워크로드를 시험하려면?**
A: `standalone_example.cpp`의 요청 제출 루프를 수정하여 원하는 접근 패턴을 구현하세요.

**Q: Gem5 예제가 실행되지 않습니다.**
A: `gem5_integration_example.cpp`는 의사 코드 참고용입니다. 실제 Gem5 통합은 Gem5 빌드 시스템이 필요합니다.

**Q: 통계가 예상과 다릅니다.**
A: 캐시 크기, 정책, 프리페처 설정을 확인하세요. 워크로드 특성에 따라 성능이 크게 달라질 수 있습니다.

## 🤝 기여

개선 사항이나 새로운 예제가 있으시면:
1. 이슈를 생성하거나
2. Pull Request를 제출해주세요

## 📄 라이선스

MIT License (MQSim_CXL과 동일)
