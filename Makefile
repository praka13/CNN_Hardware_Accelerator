################################################################################
# Makefile for CNN Inference Engine HLS Project
# Xilinx Vitis HLS Automation
################################################################################

# Project configuration
PROJECT = cnn_inference_engine
SOLUTION = solution1
TCL_SCRIPT = scripts/build_hls.tcl

# Vitis HLS command (update path if needed)
VITIS_HLS = vitis_hls

# Default target
.PHONY: all
all: csim synth

################################################################################
# Help target
################################################################################

.PHONY: help
help:
	@echo "=================================================="
	@echo "CNN Inference Engine - HLS Build System"
	@echo "=================================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make help      - Display this help message"
	@echo "  make csim      - Run C simulation only"
	@echo "  make synth     - Run C synthesis"
	@echo "  make cosim     - Run C/RTL co-simulation"
	@echo "  make export    - Export RTL as IP"
	@echo "  make all       - Run csim + synth"
	@echo "  make full      - Run complete flow (csim + synth + cosim + export)"
	@echo "  make clean     - Remove generated files"
	@echo "  make info      - Display project information"
	@echo ""
	@echo "=================================================="

################################################################################
# Information
################################################################################

.PHONY: info
info:
	@echo "=================================================="
	@echo "Project Information"
	@echo "=================================================="
	@echo "Project Name:    $(PROJECT)"
	@echo "Solution Name:   $(SOLUTION)"
	@echo "Top Function:    cnn_inference_engine"
	@echo "Target Device:   Xilinx Zynq UltraScale+ ZCU102"
	@echo "Clock Target:    200 MHz (5ns period)"
	@echo ""
	@echo "PE Array:        24×36 = 864 PEs"
	@echo "Data Type:       ap_fixed<16,8>"
	@echo "Weight Memory:   256 weights per PE"
	@echo "Line Memory:     512 pixels width"
	@echo "=================================================="

################################################################################
# C Simulation
################################################################################

.PHONY: csim
csim:
	@echo "Running C Simulation..."
	@$(VITIS_HLS) -f $(TCL_SCRIPT) -s csim
	@echo "C Simulation complete. Check $(PROJECT)/$(SOLUTION)/csim/report/"

################################################################################
# C Synthesis
################################################################################

.PHONY: synth
synth:
	@echo "Running C Synthesis..."
	@$(VITIS_HLS) -f $(TCL_SCRIPT) -s synth
	@echo "Synthesis complete. Check $(PROJECT)/$(SOLUTION)/syn/report/"

################################################################################
# C/RTL Co-Simulation
################################################################################

.PHONY: cosim
cosim:
	@echo "Running C/RTL Co-Simulation..."
	@echo "WARNING: This may take significant time for large designs."
	@$(VITIS_HLS) -f $(TCL_SCRIPT) -s cosim
	@echo "Co-simulation complete. Check $(PROJECT)/$(SOLUTION)/sim/report/"

################################################################################
# Export RTL
################################################################################

.PHONY: export
export:
	@echo "Exporting RTL IP..."
	@$(VITIS_HLS) -f $(TCL_SCRIPT) -s export
	@echo "Export complete. Check $(PROJECT)/$(SOLUTION)/impl/ip/"

################################################################################
# Complete Flow
################################################################################

.PHONY: full
full:
	@echo "Running complete HLS flow..."
	@$(VITIS_HLS) -f $(TCL_SCRIPT)
	@echo "Complete flow finished."

################################################################################
# View Reports
################################################################################

.PHONY: reports
reports:
	@echo "Opening synthesis reports..."
	@if [ -d "$(PROJECT)/$(SOLUTION)/syn/report" ]; then \
		echo "Synthesis Reports:"; \
		ls -lh $(PROJECT)/$(SOLUTION)/syn/report/; \
	else \
		echo "No synthesis reports found. Run 'make synth' first."; \
	fi

.PHONY: timing
timing:
	@echo "Timing Report:"
	@if [ -f "$(PROJECT)/$(SOLUTION)/syn/report/cnn_inference_engine_csynth.rpt" ]; then \
		grep -A 20 "Timing" $(PROJECT)/$(SOLUTION)/syn/report/cnn_inference_engine_csynth.rpt; \
	else \
		echo "Timing report not found. Run 'make synth' first."; \
	fi

.PHONY: resources
resources:
	@echo "Resource Utilization Report:"
	@if [ -f "$(PROJECT)/$(SOLUTION)/syn/report/cnn_inference_engine_csynth.rpt" ]; then \
		grep -A 30 "Utilization" $(PROJECT)/$(SOLUTION)/syn/report/cnn_inference_engine_csynth.rpt; \
	else \
		echo "Resource report not found. Run 'make synth' first."; \
	fi

################################################################################
# Clean
################################################################################

.PHONY: clean
clean:
	@echo "Cleaning generated files..."
	@rm -rf $(PROJECT)
	@rm -rf *.log
	@rm -rf *.jou
	@echo "Clean complete."

################################################################################
# Windows-specific targets (PowerShell)
################################################################################

.PHONY: win-csim
win-csim:
	@echo "Running C Simulation (Windows)..."
	@powershell -Command "& { vitis_hls -f $(TCL_SCRIPT) -s csim }"

.PHONY: win-synth
win-synth:
	@echo "Running C Synthesis (Windows)..."
	@powershell -Command "& { vitis_hls -f $(TCL_SCRIPT) -s synth }"

.PHONY: win-full
win-full:
	@echo "Running complete HLS flow (Windows)..."
	@powershell -Command "& { vitis_hls -f $(TCL_SCRIPT) }"

.PHONY: win-clean
win-clean:
	@echo "Cleaning generated files (Windows)..."
	@powershell -Command "& { Remove-Item -Recurse -Force $(PROJECT) -ErrorAction SilentlyContinue }"
	@powershell -Command "& { Remove-Item *.log -ErrorAction SilentlyContinue }"
	@powershell -Command "& { Remove-Item *.jou -ErrorAction SilentlyContinue }"
	@echo "Clean complete."
