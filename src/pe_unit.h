/******************************************************************************
 * @file pe_unit.h
 * @brief Processing Element (PE) header file
 * @description Single PE with MAC, MAX, MIN, SZD, IDM, and weight memory
 ******************************************************************************/

#ifndef PE_UNIT_H
#define PE_UNIT_H

#include "../include/cnn_types.h"

/******************************************************************************
 * PE SUB-COMPONENTS
 ******************************************************************************/

// MAC Unit: Multiply-Accumulate with 16-bit precision
inline data_t mac_unit(data_t input, data_t weight, data_t accumulator, bool reset) {
    #pragma HLS INLINE
    #pragma HLS PIPELINE II=1
    
    if (reset) {
        return input * weight;
    } else {
        return accumulator + (input * weight);
    }
}

// MAX Module: For max pooling
inline data_t max_module(data_t a, data_t b) {
    #pragma HLS INLINE
    return (a > b) ? a : b;
}

// MIN Module: For ReLU6 clipping
inline data_t min_module(data_t a, data_t b) {
    #pragma HLS INLINE
    return (a < b) ? a : b;
}

// Sign-and-Zero Detector (SZD): Checks sign and detects zeros
struct SZDResult {
    bool is_negative;
    bool is_zero;
};

inline SZDResult szd_detector(data_t value) {
    #pragma HLS INLINE
    
    SZDResult result;
    result.is_negative = (value < 0);
    result.is_zero = (value == 0);
    return result;
}

// ReLU with SZD
inline data_t relu_with_szd(data_t value) {
    #pragma HLS INLINE
    
    SZDResult szd = szd_detector(value);
    return szd.is_negative ? TO_FIXED(0) : value;
}

// ReLU6 with SZD and MIN
inline data_t relu6_with_szd(data_t value) {
    #pragma HLS INLINE
    
    data_t relu_out = relu_with_szd(value);
    return min_module(relu_out, TO_FIXED(6));
}

/******************************************************************************
 * PE UNIT CLASS
 ******************************************************************************/

class PE {
private:
    // Weight memory: Stores z weights
    data_t weight_memory[WEIGHT_MEM_DEPTH];
    
    // Accumulator register
    data_t accumulator;
    
    // Weight address counter
    addr_t weight_addr;
    
    // Input data monitor counter
    ap_uint<16> input_count;
    
    // Current state
    bool computing;
    
public:
    // Constructor
    PE() {
        #pragma HLS ARRAY_PARTITION variable=weight_memory cyclic factor=4
        #pragma HLS RESOURCE variable=weight_memory core=RAM_2P_BRAM
        
        accumulator = 0;
        weight_addr = 0;
        input_count = 0;
        computing = false;
    }
    
    // Load weight into memory
    void load_weight(data_t weight, addr_t addr) {
        #pragma HLS INLINE off
        weight_memory[addr] = weight;
    }
    
    // Process one computation cycle
    void compute(
        data_t input_data[M_SIZE],      // Inputs from m line memories
        ap_uint<5> line_selection,      // Which line to select (0 to M_SIZE-1)
        bool mac_max_mode,              // true=MAC, false=MAX
        bool sign_override,             // Sign override for first layer
        data_t bias_psum,              // Bias or partial sum input
        bool enable,                    // Enable computation
        bool reset,                     // Reset accumulator
        data_t &output,                // Output activation/partial sum
        bool &stride_request,           // Request next stride
        bool &valid                     // Output valid
    );
    
    // Reset PE state
    void reset_pe() {
        #pragma HLS INLINE
        accumulator = 0;
        weight_addr = 0;
        input_count = 0;
        computing = false;
    }
};

/******************************************************************************
 * STANDALONE PE FUNCTION (for HLS top-level)
 ******************************************************************************/

void pe_unit(
    data_t I[M_SIZE],               // Inputs from m line memories
    data_t W,                       // Weight input (streamed)
    data_t B_Psum,                  // Bias or partial sum
    ap_uint<5> line_selection,      // Line memory selector
    bool mac_max_mode,              // true=MAC, false=MAX
    bool sign_override,             // Sign override
    bool enable,                    // Enable
    bool reset,                     // Reset
    data_t &AC_Psum,               // Output
    bool &stride_request,           // Stride request
    bool &valid                     // Valid output
);

#endif // PE_UNIT_H
