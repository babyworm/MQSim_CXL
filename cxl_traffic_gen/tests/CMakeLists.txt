cmake_minimum_required(VERSION 3.10)

# Test executable
add_executable(test_cxl_traffic_patterns
    test_cxl_traffic_patterns.cpp
)

target_include_directories(test_cxl_traffic_patterns
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# No need to link cxl_traffic_gen library for this test
# (it only uses the flit structure definitions)

# Add test to CTest
add_test(NAME CXLTrafficPatterns
         COMMAND test_cxl_traffic_patterns)
