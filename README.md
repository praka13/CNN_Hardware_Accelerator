# CNN Inference Engine - Xilinx Vitis HLS Implementation

## 📋 Project Overview

This project implements a complete **energy-efficient, high-throughput CNN inference engine** in Xilinx Vitis HLS C/C++ based on research specifications. The design targets edge applications with optimized hardware acceleration for deep learning inference.

### Key Features

- ✅ **864 Processing Elements (24×36 PE Array)** with parallel MAC operations
- ✅ **Hardware-Efficient Architecture** with memory sharing and data reuse
- ✅ **Uninterrupted Processing** with intelligent pre-fetch mechanisms
- ✅ **Simplified Classification** using compare-and-select (no division/exponential)
- ✅ **Multiple Operation Support**: Conv, FC, MaxPool, AvgPool, ReLU, ReLU6
- ✅ **Flexible Kernel Sizes**: 1×1, 3×3, 5×5, 7×7
- ✅ **Fixed-Point Precision**: 16-bit (ap_fixed<16,8>)
- ✅ **AXI4-Stream & AXI4-Lite Interfaces** for SoC integration

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                   CNN INFERENCE ENGINE                          │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Inference Engine Controller (IEC)                       │  │
│  │  - Layer Scheduling                                      │  │
│  │  - Pre-fetch Control (rl-based)                          │  │
│  │  - Iteration Management (nl)                             │  │
│  └───────────────────────────────────────────────────────────┘  │
│                           │                                      │
│                           ▼                                      │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Kernel Processing Unit (KPU)                            │  │
│  │  ┌─────────────────┐  ┌─────────────────┐                │  │
│  │  │  PE Array       │  │  Line Memories  │                │  │
│  │  │  24×36 = 864    │  │  m=24 Instances │                │  │
│  │  │  Processing     │◄─┤  Data Reuse     │                │  │
│  │  │  Elements       │  │  n Outputs Each │                │  │
│  │  └─────────────────┘  └─────────────────┘                │  │
│  │            ▲                   ▲                           │  │
│  │            └───────────────────┘                           │  │
│  │         KPC Controller (FSM)                              │  │
│  └───────────────────────────────────────────────────────────┘  │
│                           │                                      │
│                           ▼                                      │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Classify Unit (CU)                                       │  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                    │  │
│  │  │ DSR  │→│ CUC  │→│ CNG  │→│ ACSU │                    │  │
│  │  └──────┘ └──────┘ └──────┘ └──────┘                    │  │
│  │  Data Router  Controller  Class#Gen  Argmax Search       │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
CNN_Hardware_Accelerator/
├── include/
│   └── cnn_types.h              # Data types, constants, configurations
├── src/
│   ├── pe_unit.h/.cpp           # Processing Element implementation
│   ├── line_memory.h/.cpp       # Line memory with data reuse
│   ├── kpc_controller.h/.cpp    # Kernel Processing Controller FSM
│   ├── pe_array.h/.cpp          # PE array + line memory integration
│   ├── classify_unit.h/.cpp     # Complete classification unit
│   ├── iec_controller.h/.cpp    # Inference Engine Controller
│   └── cnn_inference_engine.h/.cpp  # Top-level integration
├── test/
│   └── testbench.cpp            # Comprehensive test suite
├── scripts/
│   └── build_hls.tcl            # Vitis HLS build automation
├── Makefile                     # Build automation
└── README.md                    # This file
```

---

## 🔧 System Specifications

### Default Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| **PE Array Size** | 24×36 | 864 total processing elements |
| **Data Type** | ap_fixed<16,8> | 16-bit fixed-point (8 integer, 8 fractional) |
| **Weight Memory/PE** | 256 weights | z=256, 16-bit per weight |
| **Line Memory Width** | 512 pixels | Maximum feature map width |
| **Max Layers** | 50 | Supports up to 50 CNN layers |
| **Max Classes** | 1000 | ImageNet-1K classification |
| **Target Device** | xczu9eg-ffvb1156-2-e | Xilinx Zynq UltraScale+ ZCU102 |
| **Clock Target** | 200 MHz | 5ns period (conservative) |

### Supported Operations

- **Convolution**: 1×1, 3×3, 5×5, 7×7 kernels
- **Fully Connected**: Dense layers  
- **Max Pooling**: 2×2, 3×3, etc.
- **Average Pooling**: With bit-shift division
- **ReLU**: Standard rectified linear unit
- **ReLU6**: Clipped activation (0 to 6)

---

## 🚀 Getting Started

### Prerequisites

- **Xilinx Vitis HLS** 2020.2 or later
- **C++11 compatible compiler** (for testbench)
- **Make** (optional, for build automation)

### Quick Start

1. **Clone or navigate to project directory**:
   ```bash
   cd C:/Users/HP/Desktop/CNN_Hardware_Accelerator
   ```

2. **View available build targets**:
   ```bash
   make help
   ```

3. **Run C Simulation** (functional verification):
   ```bash
   make csim
   ```

4. **Run C Synthesis** (generate RTL):
   ```bash
   make synth
   ```

5. **View synthesis reports**:
   ```bash
   make reports
   make timing
   make resources
   ```

### Windows Users

Use PowerShell-compatible targets:
```powershell
make win-csim     # C Simulation
make win-synth    # C Synthesis
make win-full     # Complete flow
make win-clean    # Clean project
```

### Manual Build (Vitis HLS GUI)

1. Open Vitis HLS
2. **File → New Project**
3. Set project name: `cnn_inference_engine`
4. Add all files from `src/` directory
5. Add testbench from `test/testbench.cpp`
6. Set top function: `cnn_inference_engine`
7. Set part: `xczu9eg-ffvb1156-2-e`
8. Run synthesis and simulation

---

## 🧪 Test Cases

The testbench includes **7 comprehensive test cases**:

### Test 1: 3×3 Convolution
- Input: 5×5 feature map
- Kernel: 3×3 edge detection filter
- Output: 3×3 feature map
- **Purpose**: Verify convolution operation

### Test 2: Fully Connected Layer
- Input: 4 neurons
- Output: 3 neurons
- **Purpose**: Verify matrix multiplication and bias addition

### Test 3: Max Pooling 2×2
- Input: 4×4 feature map
- Pool size: 2×2, stride=2
- Output: 2×2 feature map
- **Purpose**: Verify pooling operation

### Test 4: ReLU Activation
- Tests positive, negative, and zero inputs
- **Purpose**: Verify rectified linear unit

### Test 5: ReLU6 Activation
- Tests clipping at value 6
- **Purpose**: Verify bounded activation

### Test 6: Classification (ACSU)
- Simulates 10-class classification
- **Purpose**: Verify argmax without division/exponential

### Test 7: Multi-Layer CNN
- 3-layer network: Conv → MaxPool → FC
- **Purpose**: Verify end-to-end inference pipeline

---

## 📊 Performance Metrics

### Theoretical Performance (from Paper)

- **Throughput**: 6.65 TOPs (with 864 PEs @ 1.25 GHz FPGA)
- **Power Efficiency**: Energy-efficient design with power gating
- **Data Reuse**: Each input reused z×(α-s)² times
- **Weight Reuse**: Each weight reused (A-s)² times

### HLS Synthesis Targets

- **Clock**: 200 MHz (achievable on ZCU102)
- **Latency**: Layer-dependent, minimized through pipelining
- **Resource Usage** (estimated):
  - DSP48E2: ~864 (one per PE for MAC)
  - BRAM: Depends on line memory size
  - LUT/FF: Controller and routing logic

---

## 🎯 Algorithm Implementation

The system implements this processing algorithm:

```
Initialization:
  i = 1, j = 0, l = 0
  ACMax = 0, CN-DC = 0

FOR each layer l:
  1. Configure layer (type, kernel, stride, etc.)
  2. Determine FClast, nl, rl
  
  FOR each iteration i (1 to nl):
    3. Pre-fetch rl data items
    4. Wait until j >= rl (enough data)
    5. Start processing while continuing to fetch
    6. If current fetch complete, pre-fetch for next iteration
    7. When processing complete:
       - If l == FClast:
           * Compare ACi with ACMax
           * Update ACMax and CN-DC if ACi > ACMax
       - Else:
           * Output activation or partial sum
    8. Transition to next iteration or layer

9. If l == FClast AND i == nl:
     Output CN-DC (final classification)
```

---

## 🔍 Key Design Features

### 1. Uninterrupted Processing
- Pre-fetch `rl` data items before computation starts
- Continue fetching remaining data during computation
- Start pre-fetch for iteration i+1 before i completes
- **Result**: Zero idle cycles, maximum throughput

### 2. Data Reuse
- **Input Reuse**: Each pixel shared across z×(α-s)² operations
- **Weight Reuse**: Each weight shared across (A-s)² operations
- **Line Memory Sharing**: n PEs share same line memory
- **Vertical Stride Reuse**: α-s line memories reused
- **Result**: Reduced memory bandwidth by orders of magnitude

### 3. Hardware-Efficient Classification
- **No Softmax**: Avoids exponential computation
- **No Division**: Uses compare-and-select for argmax
- **Single Pass**: O(N) complexity for N classes
- **Registers Only**: REG1 (ACMax), REG2 (CN-DC)
- **Result**: Minimal hardware overhead

### 4. Dynamic Reconfiguration
- Runtime layer configuration via AXI4-Lite
- Support different operation types per layer
- Flexible kernel sizes without hardware changes
- **Result**: Single accelerator for entire CNN

---

## 📈 Usage Example

```cpp
// Configure layers
LayerConfig layers[3];

// Layer 0: Conv 3×3
layers[0].layer_type = CONV;
layers[0].kernel_h = 3; layers[0].kernel_w = 3;
layers[0].input_h = 32; layers[0].input_w = 32;
layers[0].output_h = 30; layers[0].output_w = 30;
// ... set other parameters

// Layer 1: MaxPool 2×2
layers[1].layer_type = MAXPOOL;
// ... configure

// Layer 2: FC (classification)
layers[2].layer_type = FC;
layers[2].is_fc_last = true;
layers[2].num_classes = 10;

// Create streams
hls::stream<data_t> input, weights, bias, output;

// Load data into streams
// ... (load image, weights)

// Run inference
bool done = false;
int detected_class = -1;

cnn_inference_engine(
    input, weights, bias, output,
    layers, 3,  // 3 layers
    true,       // start
    done, interrupt,
    detected_class,
    layer, iter, cycles
);

// Result
std::cout << "Detected class: " << detected_class << std::endl;
```

---

## 🐛 Troubleshooting

### Common Issues

**Issue**: Synthesis fails with timing violations
- **Solution**: Increase clock period in `build_hls.tcl` (e.g., 6ns = 166 MHz)

**Issue**: Resource utilization too high
- **Solution**: Reduce PE array size in `cnn_types.h` (e.g., M_SIZE=16, N_SIZE=24)

**Issue**: C simulation hangs
- **Solution**: Reduce test data size in testbench, check stream read/write balance

**Issue**: Incorrect results in fixed-point
- **Solution**: Adjust INT_BITS/FRAC_BITS ratio in `cnn_types.h` based on data range

---

## 📝 Customization

### Changing PE Array Size

Edit `include/cnn_types.h`:
```cpp
#define M_SIZE 16   // Rows (was 24)
#define N_SIZE 24   // Columns (was 36)
```

### Changing Data Precision

Edit `include/cnn_types.h`:
```cpp
#define DATA_WIDTH 16
#define INT_BITS 10    // More integer bits
#define FRAC_BITS 6    // Fewer fractional bits
```

### Changing Target Device

Edit `scripts/build_hls.tcl`:
```tcl
set part "xczu7ev-ffvc1156-2-e"  # Different Zynq device
```

---

## 📚 References

- Research Paper: *Energy-Efficient and High-Throughput CNN Inference Engine Based on Memory-Sharing and Data-Reusing*
- Xilinx Vitis HLS User Guide (UG1399)
- Xilinx Zynq UltraScale+ Documentation

---

## 👥 Contributors

- **Implementation**: CNN Hardware Accelerator Team
- **Research Base**: Paper authors
- **Target Platform**: Xilinx Zynq UltraScale+ ZCU102

---

## 📄 License

This implementation is for educational and research purposes.

---

## 🎓 Presentation Notes

This codebase is suitable for presentations with:

✅ **7 Working Test Cases** demonstrating all operations  
✅ **Clear Console Output** with formatted results  
✅ **Complete Documentation** explaining architecture  
✅ **Synthesis Reports** showing resource usage and timing  
✅ **Modular Structure** easy to explain component-by-component  

### Recommended Presentation Flow

1. **Overview**: Show architecture diagram
2. **Demo**: Run `make csim` to show test results
3. **Deep Dive**: Explain one component (e.g., PE or ACSU)
4. **Results**: Show synthesis reports (timing, resources)
5. **Conclusion**: Highlight key innovations (data reuse, uninterrupted processing)

---

**For questions or issues, please refer to the source code comments or contact the development team.**
