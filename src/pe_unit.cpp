/******************************************************************************
 * @file pe_unit.cpp
 * @brief Processing Element (PE) implementation
 * @description Complete PE with MAC, MAX, MIN, SZD, weight memory, and control logic
 ******************************************************************************/

#include "pe_unit.h"

/******************************************************************************
 * PE CLASS IMPLEMENTATION
 ******************************************************************************/

void PE::compute(
    data_t input_data[M_SIZE],
    ap_uint<5> line_selection,
    bool mac_max_mode,
    bool sign_override,
    data_t bias_psum,
    bool enable,
    bool reset_acc,
    data_t &output,
    bool &stride_request,
    bool &valid
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    #pragma HLS ARRAY_PARTITION variable=input_data complete
    
    // Initialize outputs
    valid = false;
    stride_request = false;
    output = 0;
    
    if (!enable) {
        return;
    }
    
    // Reset handling
    if (reset_acc) {
        accumulator = bias_psum;  // Initialize with bias
        weight_addr = 0;
        input_count = 0;
        computing = true;
        return;
    }
    
    if (!computing) {
        return;
    }
    
    // Line selection MUX: Select input from one of m line memories
    data_t selected_input = input_data[line_selection];
    
    // Fetch weight from weight memory
    data_t current_weight = weight_memory[weight_addr];
    
    if (mac_max_mode) {
        // MAC Mode: Multiply-Accumulate
        accumulator = mac_unit(selected_input, current_weight, accumulator, false);
        
        // Increment weight address
        weight_addr++;
        
        // Check if we need more input data (stride request)
        input_count++;
        if (input_count % N_SIZE == 0) {  // After processing n inputs
            stride_request = true;
        }
        
    } else {
        // MAX Mode: Max pooling
        accumulator = max_module(accumulator, selected_input);
        stride_request = true;  // Always request next in pooling
    }
    
    // Output generation (when computation for this output is complete)
    // This happens after processing all required inputs for one output
    bool computation_complete = (weight_addr >= WEIGHT_MEM_DEPTH) || 
                                (input_count >= N_SIZE && !mac_max_mode);
    
    if (computation_complete) {
        // Apply activation if needed
        SZDResult szd = szd_detector(accumulator);
        
        if (sign_override) {
            // First layer: keep original value
            output = accumulator;
        } else {
            // Apply ReLU (zero out negative values)
            output = szd.is_negative ? TO_FIXED(0) : accumulator;
        }
        
        valid = true;
        computing = false;  // Ready for next computation
    }
}

/******************************************************************************
 * STANDALONE PE FUNCTION
 ******************************************************************************/

void pe_unit(
    data_t I[M_SIZE],
    data_t W,
    data_t B_Psum,
    ap_uint<5> line_selection,
    bool mac_max_mode,
    bool sign_override,
    bool enable,
    bool reset,
    data_t &AC_Psum,
    bool &stride_request,
    bool &valid
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    #pragma HLS ARRAY_PARTITION variable=I complete
    
    // Static PE instance (maintains state across calls)
    static PE pe_instance;
    #pragma HLS RESET variable=pe_instance
    
    // Weight loading mode vs compute mode
    static bool weight_load_mode = false;
    static addr_t weight_load_addr = 0;
    
    // If weight input is valid (non-zero or explicit load signal), load it
    // In actual implementation, you'd have a separate control signal
    // For simplicity, we assume weights are pre-loaded
    
    // Compute operation
    pe_instance.compute(
        I,
        line_selection,
        mac_max_mode,
        sign_override,
        B_Psum,
        enable,
        reset,
        AC_Psum,
        stride_request,
        valid
    );
}

/******************************************************************************
 * Address Generation Unit (AGU) for Weight Memory
 ******************************************************************************/

class WeightAGU {
private:
    addr_t read_addr;
    addr_t write_addr;
    
public:
    WeightAGU() : read_addr(0), write_addr(0) {
        #pragma HLS RESET variable=read_addr
        #pragma HLS RESET variable=write_addr
    }
    
    addr_t get_read_addr(bool increment) {
        #pragma HLS INLINE
        addr_t current = read_addr;
        if (increment) {
            read_addr++;
            if (read_addr >= WEIGHT_MEM_DEPTH) {
                read_addr = 0;  // Wrap around
            }
        }
        return current;
    }
    
    addr_t get_write_addr(bool increment) {
        #pragma HLS INLINE
        addr_t current = write_addr;
        if (increment) {
            write_addr++;
        }
        return current;
    }
    
    void reset() {
        #pragma HLS INLINE
        read_addr = 0;
        write_addr = 0;
    }
};

/******************************************************************************
 * Input Data Monitor (IDM) for PE
 ******************************************************************************/

class InputDataMonitor {
private:
    ap_uint<16> data_count;
    ap_uint<16> required_count;
    bool ready;
    
public:
    InputDataMonitor() : data_count(0), required_count(0), ready(false) {}
    
    void set_required(ap_uint<16> required) {
        #pragma HLS INLINE
        required_count = required;
        data_count = 0;
        ready = false;
    }
    
    void increment() {
        #pragma HLS INLINE
        if (!ready) {
            data_count++;
            if (data_count >= required_count) {
                ready = true;
            }
        }
    }
    
    bool is_ready() {
        #pragma HLS INLINE
        return ready;
    }
    
    ap_uint<16> get_count() {
        #pragma HLS INLINE
        return data_count;
    }
};
