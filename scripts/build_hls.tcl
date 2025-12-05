################################################################################
# @file build_hls.tcl
# @brief Xilinx Vitis HLS build script for CNN Inference Engine
# @description Automated synthesis, simulation, and export
################################################################################

# Project settings
set project_name "cnn_inference_engine"
set top_function "cnn_inference_engine"
set solution_name "solution1"

# Target device: Xilinx Zynq UltraScale+ ZCU102
set part "xczu1cg-sbva484-1-e"

# Clock period: 5ns = 200 MHz (conservative for ZCU102)
set clock_period 5

################################################################################
# Create Project
################################################################################

# Remove existing project if it exists
if {[file exists $project_name]} {
    file delete -force $project_name
}

# Create new project
open_project $project_name
set_top $top_function

# Add source files
add_files src/cnn_inference_engine.cpp -cflags "-I./include -std=c++11"
add_files src/iec_controller.cpp -cflags "-I./include -std=c++11"
add_files src/pe_array.cpp -cflags "-I./include -std=c++11"
add_files src/kpc_controller.cpp -cflags "-I./include -std=c++11"
add_files src/pe_unit.cpp -cflags "-I./include -std=c++11"
add_files src/line_memory.cpp -cflags "-I./include -std=c++11"
add_files src/classify_unit.cpp -cflags "-I./include -std=c++11"

# Add testbench
add_files -tb test/testbench.cpp -cflags "-I./include -I./src -std=c++11"

################################################################################
# Create Solution
################################################################################

open_solution $solution_name

# Set device
set_part $part

# Create clock
create_clock -period $clock_period -name default

# Configuration
config_compile -name_max_length 256
config_rtl -reset all -reset_async

# Interface configuration
config_interface -m_axi_latency 64
config_interface -m_axi_max_widen_bitwidth 512

################################################################################
# C Simulation (Optional - can be run separately)
################################################################################

puts "\n=========================================="
puts "Running C Simulation..."
puts "==========================================\n"

# Run C simulation
csim_design -clean

puts "\n=========================================="
puts "C Simulation Complete"
puts "==========================================\n"

################################################################################
# C Synthesis
################################################################################

puts "\n=========================================="
puts "Running C Synthesis..."
puts "==========================================\n"

# Run synthesis
csynth_design

puts "\n=========================================="
puts "C Synthesis Complete"
puts "==========================================\n"

# Generate detailed reports
puts "Generating synthesis reports..."

################################################################################
# C/RTL Co-Simulation (Optional - can be slow)
################################################################################

# Uncomment to run co-simulation
# puts "\n=========================================="
# puts "Running C/RTL Co-Simulation..."
# puts "==========================================\n"
# 
# cosim_design -trace_level all
# 
# puts "\n=========================================="
# puts "Co-Simulation Complete"
# puts "==========================================\n"

################################################################################
# Export RTL
################################################################################

puts "\n=========================================="
puts "Exporting RTL Design..."
puts "==========================================\n"

# Export as IP catalog (Vivado IP)
export_design -format ip_catalog -description "CNN Inference Engine IP" -vendor "user" -version "1.0"

puts "\n=========================================="
puts "Export Complete"
puts "==========================================\n"

################################################################################
# Summary Report
################################################################################

puts "\n=========================================="
puts "BUILD SUMMARY"
puts "==========================================\n"

puts "Project: $project_name"
puts "Solution: $solution_name"
puts "Top Function: $top_function"
puts "Target Device: $part"
puts "Clock Period: ${clock_period}ns (200 MHz)"
puts ""
puts "Outputs:"
puts "  - Synthesis report: $project_name/$solution_name/syn/report/"
puts "  - RTL files: $project_name/$solution_name/syn/verilog/"
puts "  - IP package: $project_name/$solution_name/impl/ip/"
puts ""
puts "==========================================\n"

# Exit
exit
