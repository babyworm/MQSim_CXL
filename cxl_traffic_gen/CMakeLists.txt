cmake_minimum_required(VERSION 3.10)
project(CXLTrafficGenerator VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(-O3)
    endif()
endif()

# =============================================================================
# Source Files
# =============================================================================

# CXL Layer
set(CXL_SOURCES
    src/cxl/CXL_Config.cpp
    src/cxl/CXL_MSHR.cpp
    src/cxl/DRAM_Model.cpp
    src/cxl/DRAM_Subsystem.cpp
    src/cxl/Host_Interface_CXL.cpp
    src/cxl/Prefetching_Alg.cpp
    src/cxl/CFLRU.cpp
    src/cxl/lrfu_heap.cpp
)

# Simulation Engine
set(SIM_SOURCES
    src/sim/Engine.cpp
    src/sim/EventTree.cpp
    src/sim/Sim_Reporter.cpp
)

# SSD Layer (will be populated when actual implementation is done)
set(SSD_SOURCES
    # src/ssd/NVM_Firmware.cpp
    # src/ssd/Address_Mapping_Unit_Page_Level.cpp
    # src/ssd/Flash_Block_Manager.cpp
    # src/ssd/GC_and_WL_Unit_Page_Level.cpp
    # src/ssd/TSU_OutofOrder.cpp
    # src/ssd/NVM_PHY_ONFI_NVDDR2.cpp
    # src/ssd/ONFI_Channel_NVDDR2.cpp
    # ... (add more as needed)
)

# Flash Chip Layer
set(FLASH_SOURCES
    # src/flash/Flash_Chip.cpp
    # src/flash/Die.cpp
    # src/flash/Plane.cpp
    # src/flash/Block.cpp
    # src/flash/Physical_Page_Address.cpp
    # ... (add more as needed)
)

# Core Traffic Generator
set(CORE_SOURCES
    src/core/traffic_generator.cpp
)

# Utilities
set(UTILS_SOURCES
    # src/utils/Logical_Address_Partitioning_Unit.cpp
    # ... (add minimal utilities)
)

# =============================================================================
# Library Target
# =============================================================================

add_library(cxl_traffic_gen
    ${CXL_SOURCES}
    ${SIM_SOURCES}
    ${SSD_SOURCES}
    ${FLASH_SOURCES}
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
)

target_include_directories(cxl_traffic_gen
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# =============================================================================
# Examples
# =============================================================================

# Standalone Example
add_executable(standalone_example
    examples/standalone_example.cpp
)
target_link_libraries(standalone_example
    PRIVATE cxl_traffic_gen
)

# Gem5 Integration Example
add_executable(gem5_integration_example
    examples/gem5_integration_example.cpp
)
target_link_libraries(gem5_integration_example
    PRIVATE cxl_traffic_gen
)

# =============================================================================
# Installation
# =============================================================================

include(GNUInstallDirs)

# Install library
install(TARGETS cxl_traffic_gen
    EXPORT CXLTrafficGeneratorTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install examples
install(TARGETS standalone_example gem5_integration_example
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Export targets
install(EXPORT CXLTrafficGeneratorTargets
    FILE CXLTrafficGeneratorTargets.cmake
    NAMESPACE CXL::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CXLTrafficGenerator
)

# Create package config
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CXLTrafficGeneratorConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CXLTrafficGeneratorConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CXLTrafficGenerator
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CXLTrafficGeneratorConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CXLTrafficGeneratorConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CXLTrafficGeneratorConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CXLTrafficGenerator
)

# =============================================================================
# Testing (optional)
# =============================================================================

option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# =============================================================================
# Summary
# =============================================================================

message(STATUS "")
message(STATUS "CXL Traffic Generator Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build Testing: ${BUILD_TESTING}")
message(STATUS "")
