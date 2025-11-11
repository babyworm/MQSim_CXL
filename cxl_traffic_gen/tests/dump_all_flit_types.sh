#!/bin/bash
#
# CXL Flit Type Dumper
# Generates hex dumps for all CXL flit types
#

set -e

GENERATOR="./cxl_flit_generator"
OUTPUT_DIR="./flit_dumps"

# Check if generator exists
if [ ! -f "$GENERATOR" ]; then
    echo "Error: $GENERATOR not found. Please run 'make' first."
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "================================================================================"
echo "              CXL Flit Type Dump Generator                                     "
echo "================================================================================"
echo ""
echo "Generating individual hex dumps for each CXL flit type..."
echo "Output directory: $OUTPUT_DIR"
echo ""

# List of all flit types
FLIT_TYPES=(
    "mem_rd"
    "mem_rd_data"
    "mem_wr"
    "mem_wr_ptl"
    "mem_data"
    "mem_data_nxm"
    "cpl"
    "cpl_data"
    "snp_data"
    "snp_inv"
)

# Generate dump for each type
for flit_type in "${FLIT_TYPES[@]}"; do
    output_file="$OUTPUT_DIR/${flit_type}.txt"
    echo "  [*] Generating: $flit_type -> $output_file"
    $GENERATOR "$flit_type" > "$output_file"
done

# Generate combined dump with all types
echo ""
echo "  [*] Generating combined dump: all_flit_types.txt"
$GENERATOR all > "$OUTPUT_DIR/all_flit_types.txt"

# Generate summary
echo ""
echo "================================================================================"
echo "                           Summary                                             "
echo "================================================================================"
echo ""
echo "Total flit types generated: ${#FLIT_TYPES[@]}"
echo ""
echo "Individual dumps:"
for flit_type in "${FLIT_TYPES[@]}"; do
    output_file="$OUTPUT_DIR/${flit_type}.txt"
    size=$(wc -c < "$output_file")
    echo "  - $output_file ($size bytes)"
done
echo ""
echo "Combined dump:"
output_file="$OUTPUT_DIR/all_flit_types.txt"
size=$(wc -c < "$output_file")
echo "  - $output_file ($size bytes)"
echo ""
echo "================================================================================"
echo "                        Generation Complete!                                   "
echo "================================================================================"
echo ""
echo "To view a specific flit type:"
echo "  cat $OUTPUT_DIR/mem_rd.txt"
echo ""
echo "To view all flit types:"
echo "  cat $OUTPUT_DIR/all_flit_types.txt"
echo ""
