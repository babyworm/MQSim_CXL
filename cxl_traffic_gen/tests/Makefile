# Simple Makefile for CXL Traffic Pattern Tests
# Can be used without CMake for quick testing

CXX := g++
CXXFLAGS := -std=c++14 -Wall -Wextra -O2 -g
INCLUDES := -I../include

# Targets
TARGETS := test_cxl_traffic_patterns

all: $(TARGETS)

test_cxl_traffic_patterns: test_cxl_traffic_patterns.cpp ../include/cxl_flit.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<
	@echo ""
	@echo "✅ Built: $@"
	@echo "Run with: ./$@"

clean:
	rm -f $(TARGETS)
	@echo "Cleaned test binaries"

run: test_cxl_traffic_patterns
	@echo "Running CXL Traffic Pattern Tests..."
	@echo ""
	./test_cxl_traffic_patterns

# Save output to file
run-save: test_cxl_traffic_patterns
	@echo "Running tests and saving output..."
	./test_cxl_traffic_patterns | tee test_output.txt
	@echo ""
	@echo "Output saved to test_output.txt"

.PHONY: all clean run run-save
