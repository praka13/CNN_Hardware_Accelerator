/******************************************************************************
 * @file cnn_types.h
 * @brief Core data types, constants, and configuration structures for CNN Inference Engine
 * @description Xilinx Vitis HLS implementation - ADAPTED FOR XCZU1CG
 * @device xczu1cg-sbva484-1-e (240 DSP slices)
 ******************************************************************************/

#ifndef CNN_TYPES_H
#define CNN_TYPES_H

#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"

/******************************************************************************
 * SYSTEM CONSTANTS - XCZU1CG CONFIGURATION (240 DSPs)
 ******************************************************************************/

// PE Array dimensions (m × n) - REDUCED FOR XCZU1CG
// Original: 24×36 = 864 PEs (needs 864 DSPs) - TOO LARGE
// New: 8×12 = 96 PEs (needs 96 DSPs) - FITS IN XCZU1CG
#define M_SIZE 8           // Number of rows in PE array (was 24)
#define N_SIZE 12          // Number of columns in PE array (was 36)  
#define TOTAL_PES (M_SIZE * N_SIZE)  // 96 PEs total (fits in 240 DSPs)

// Data Width Configuration
#define DATA_WIDTH 16       // 16-bit fixed-point
#define INT_BITS 8          // Integer bits
#define FRAC_BITS 8         // Fractional bits

// Memory Configuration
#define WEIGHT_MEM_DEPTH 256        // Weights per PE (z parameter)
#define LINE_MEM_WIDTH 512          // Maximum feature map width (A parameter)
#define MAX_FEATURE_MAP_SIZE 512    // Maximum H or W dimension
#define MAX_CHANNELS 1024           // Maximum number of channels

// Kernel Configuration
#define MAX_KERNEL_SIZE 7           // Maximum kernel dimension (7×7)
#define MIN_KERNEL_SIZE 1           // Minimum kernel dimension (1×1)

// Layer Configuration
#define MAX_LAYERS 50               // Maximum CNN layers

// Classification
#define MAX_CLASSES 1000            // ImageNet-1K classes

/******************************************************************************
 * DATA TYPES
 ******************************************************************************/

// Fixed-point data type: ap_fixed<16, 8> = 8 integer bits, 8 fractional bits
typedef ap_fixed<DATA_WIDTH, INT_BITS> data_t;

// Address types
typedef ap_uint<10> addr_t;         // Up to 1024 addresses
typedef ap_uint<16> large_addr_t;   // For larger memory spaces

// Index types
typedef ap_uint<8> idx_t;           // General indexing (0-255)
typedef ap_uint<12> large_idx_t;    // Larger indices (0-4095)

/******************************************************************************
 * LAYER TYPE ENUMERATION
 ******************************************************************************/

typedef enum {
    CONV = 0,       // Convolution layer
    FC = 1,         // Fully connected layer
    MAXPOOL = 2,    // Max pooling layer
    AVGPOOL = 3,    // Average pooling layer
    RELU = 4,       // ReLU activation
    RELU6 = 5       // ReLU6 activation (clipped at 6)
} layer_type_t;

/******************************************************************************
 * LAYER CONFIGURATION STRUCTURE
 ******************************************************************************/

struct LayerConfig {
    // Layer type and operation
    layer_type_t layer_type;
    
    // Kernel dimensions
    ap_uint<4> kernel_h;        // Kernel height (1, 3, 5, 7)
    ap_uint<4> kernel_w;        // Kernel width (1, 3, 5, 7)
    ap_uint<11> kernel_d;       // Kernel depth (input channels)
    ap_uint<11> num_filters;    // Number of output filters
    
    // Input dimensions
    ap_uint<10> input_h;        // Input height
    ap_uint<10> input_w;        // Input width
    ap_uint<11> input_c;        // Input channels
    
    // Output dimensions
    ap_uint<10> output_h;       // Output height
    ap_uint<10> output_w;       // Output width
    ap_uint<11> output_c;       // Output channels
    
    // Stride and padding
    ap_uint<3> stride;          // Stride (1, 2, 3, etc.)
    ap_uint<3> padding;         // Padding (0, 1, 2, 3)
    
    // Iteration control
    ap_uint<16> nl;             // Number of iterations for this layer
    ap_uint<10> rl;             // Minimum data required for pre-fetch
    
    // Classification control
    bool is_fc_last;            // True if this is the last FC layer (FClast)
    ap_uint<12> num_classes;    // Number of classes (for FClast layer)
    
    // Constructor for initialization
    LayerConfig() :
        layer_type(CONV),
        kernel_h(3), kernel_w(3), kernel_d(3), num_filters(64),
        input_h(224), input_w(224), input_c(3),
        output_h(224), output_w(224), output_c(64),
        stride(1), padding(1),
        nl(1), rl(1),
        is_fc_last(false), num_classes(1000)
    {}
};

/******************************************************************************
 * PE CONFIGURATION STRUCTURE
 ******************************************************************************/

struct PEConfig {
    // Line memory selection (which of m line memories to read from)
    ap_uint<5> line_select;     // 0 to M_SIZE-1
    
    // Operation mode
    bool mac_max_mode;          // true = MAC operation, false = MAX operation
    
    // Sign control
    bool sign_override;         // Override sign detection for first layer
    
    // Enable signals
    bool enable;                // Enable PE computation
    bool reset;                 // Reset accumulator
    
    // Constructor
    PEConfig() :
        line_select(0),
        mac_max_mode(true),
        sign_override(false),
        enable(true),
        reset(false)
    {}
};

/******************************************************************************
 * KERNEL PROCESSING CONTROLLER STATE
 ******************************************************************************/

typedef enum {
    KPC_IDLE = 0,       // Idle state
    KPC_PREFETCH = 1,   // Pre-fetching data
    KPC_COMPUTE = 2,    // Computing
    KPC_STRIDE_H = 3,   // Horizontal stride
    KPC_STRIDE_V = 4,   // Vertical stride
    KPC_DONE = 5        // Computation done
} kpc_state_t;

/******************************************************************************
 * INFERENCE ENGINE CONTROLLER STATE
 ******************************************************************************/

typedef enum {
    IEC_IDLE = 0,       // Idle, waiting for start
    IEC_CONFIG = 1,     // Loading configuration
    IEC_PREFETCH = 2,   // Pre-fetching rl data
    IEC_COMPUTE = 3,    // Processing while fetching
    IEC_NEXT_ITER = 4,  // Transition to next iteration
    IEC_NEXT_LAYER = 5, // Transition to next layer
    IEC_CLASSIFY = 6,   // Classification in progress
    IEC_DONE = 7        // All layers complete
} iec_state_t;

/******************************************************************************
 * CLASSIFY UNIT CONTROLLER STATE
 ******************************************************************************/

typedef enum {
    CUC_IDLE = 0,       // Idle
    CUC_ACTIVE = 1,     // Processing activations
    CUC_DONE = 2        // Classification complete
} cuc_state_t;

/******************************************************************************
 * COMPUTATION STATISTICS (for debugging/monitoring)
 ******************************************************************************/

struct ComputeStats {
    ap_uint<32> total_cycles;
    ap_uint<32> compute_cycles;
    ap_uint<32> prefetch_cycles;
    ap_uint<16> layers_processed;
    
    ComputeStats() :
        total_cycles(0),
        compute_cycles(0),
        prefetch_cycles(0),
        layers_processed(0)
    {}
};

/******************************************************************************
 * UTILITY MACROS
 ******************************************************************************/

// Clamp value between min and max
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// ReLU activation
#define RELU_ACT(x) ((x) > 0 ? (x) : 0)

// ReLU6 activation 
#define RELU6_ACT(x) CLAMP((x), 0, 6)

// Convert to fixed-point constant
#define TO_FIXED(x) ((data_t)(x))

/******************************************************************************
 * ASSERTIONS FOR PARAMETER VALIDATION
 ******************************************************************************/

// Compile-time assertions
#define STATIC_ASSERT(condition, message) \
    typedef char static_assert_##message[(condition) ? 1 : -1]

// PE array size must fit in device (xczu1cg has 240 DSPs)
STATIC_ASSERT((M_SIZE * N_SIZE <= 200), pe_array_too_large_for_xczu1cg);

// Data width must be positive
STATIC_ASSERT((DATA_WIDTH > 0), data_width_invalid);

// M_SIZE must fit in 5 bits for addressing
STATIC_ASSERT((M_SIZE <= 32), m_size_too_large);

#endif // CNN_TYPES_H
