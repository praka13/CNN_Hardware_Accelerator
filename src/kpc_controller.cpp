/******************************************************************************
 * @file kpc_controller.cpp
 * @brief Kernel Processing Controller (KPC) implementation
 * @description FSM managing PE array, line memories, stride control, and data reuse
 ******************************************************************************/

#include "kpc_controller.h"

/******************************************************************************
 * KPC CONTROLLER IMPLEMENTATION
 ******************************************************************************/

KPCController::KPCController() {
    current_state = KPC_IDLE;
    current_row = 0;
    current_col = 0;
    iteration_count = 0;
    total_iterations = 0;
    data_fetched = 0;
    data_required = 0;
    h_stride_count = 0;
    v_stride_count = 0;
}

void KPCController::reset() {
    #pragma HLS INLINE
    
    current_state = KPC_IDLE;
    current_row = 0;
    current_col = 0;
    iteration_count = 0;
    h_stride_count = 0;
    v_stride_count = 0;
}

void KPCController::configure(LayerConfig &config) {
    #pragma HLS INLINE
    
    total_iterations = config.nl;
    data_required = config.rl;
    data_fetched = 0;
    iteration_count = 0;
    current_state = KPC_PREFETCH;
}

void KPCController::control(
    LayerConfig &config,
    bool stride_requests[M_SIZE][N_SIZE],
    ap_uint<5> line_selection[M_SIZE][N_SIZE],
    bool read_enable[M_SIZE],
    bool write_enable[M_SIZE],
    bool reuse_mode[M_SIZE],
    addr_t ra_r[M_SIZE],
    addr_t ra_n[M_SIZE],
    bool &next_stride,
    bool &compute_enable,
    bool &layer_done
) {
    #pragma HLS PIPELINE II=1
    #pragma HLS ARRAY_PARTITION variable=stride_requests complete dim=0
    #pragma HLS ARRAY_PARTITION variable=line_selection complete dim=0
    #pragma HLS ARRAY_PARTITION variable=read_enable complete
    #pragma HLS ARRAY_PARTITION variable=write_enable complete
    #pragma HLS ARRAY_PARTITION variable=reuse_mode complete
    
    // Default outputs
    next_stride = false;
    compute_enable = false;
    layer_done = false;
    
    // FSM State Machine
    switch (current_state) {
        
        case KPC_IDLE:
            // Wait for configuration
            for (int i = 0; i < M_SIZE; i++) {
                #pragma HLS UNROLL
                read_enable[i] = false;
                write_enable[i] = false;
            }
            break;
            
        case KPC_PREFETCH:
            // Pre-fetch rl data items before starting computation
            // Enable writing to line memories
            for (int i = 0; i < M_SIZE; i++) {
                #pragma HLS UNROLL
                write_enable[i] = true;
                read_enable[i] = false;
            }
            
            data_fetched++;
            
            // Check if we have enough data
            if (data_fetched >= data_required) {
                current_state = KPC_COMPUTE;
            }
            break;
            
        case KPC_COMPUTE: {
            // Enable computation
            compute_enable = true;
            
            // Continue fetching remaining data while computing
            for (int i = 0; i < M_SIZE; i++) {
                #pragma HLS UNROLL
                write_enable[i] = true;  // Continue writing
                read_enable[i] = true;   // Enable reading for PEs
            }
            
            // Generate line selection for each PE
            // Each PE in column j reads from line memory (current_row + PE_row) % M_SIZE
            for (int i = 0; i < M_SIZE; i++) {
                #pragma HLS UNROLL
                for (int j = 0; j < N_SIZE; j++) {
                    #pragma HLS UNROLL
                    
                    // Line selection based on kernel position
                    ap_uint<5> line_idx = (i + (current_row % config.kernel_h)) % M_SIZE;
                    line_selection[i][j] = line_idx;
                }
            }
            
            // Check if any PE requests stride
            bool any_stride_request = false;
            for (int i = 0; i < M_SIZE; i++) {
                #pragma HLS UNROLL
                for (int j = 0; j < N_SIZE; j++) {
                    #pragma HLS UNROLL
                    if (stride_requests[i][j]) {
                        any_stride_request = true;
                    }
                }
            }
            
            if (any_stride_request) {
                current_state = KPC_STRIDE_H;
            }
            break;
        }
            
        case KPC_STRIDE_H: {
            // Horizontal stride: Move within same line memories
            next_stride = true;
            
            current_col += config.stride;
            h_stride_count++;
            
            // Check if we reached end of row
            if (current_col >= (config.input_w - config.kernel_w + 1)) {
                current_col = 0;
                current_state = KPC_STRIDE_V;
            } else {
                // Determine reuse mode
                // If stride < kernel_w, we reuse data
                for (int i = 0; i < M_SIZE; i++) {
                    #pragma HLS UNROLL
                    if (config.stride < config.kernel_w) {
                        reuse_mode[i] = true;
                        // Set reuse addresses
                        ra_r[i] = current_col;
                    } else {
                        reuse_mode[i] = false;
                        ra_n[i] = current_col;
                    }
                }
                
                current_state = KPC_COMPUTE;
            }
            break;
        }
            
        case KPC_STRIDE_V: {
            // Vertical stride: Move to next set of line memories
            next_stride = true;
            
            current_row += config.stride;
            v_stride_count++;
            
            // Check if we completed one iteration
            if (current_row >= (config.input_h - config.kernel_h + 1)) {
                current_row = 0;
                iteration_count++;
                
                // Check if all iterations complete
                if (iteration_count >= total_iterations) {
                    current_state = KPC_DONE;
                } else {
                    // Start pre-fetch for next iteration
                    data_fetched = 0;
                    current_state = KPC_PREFETCH;
                }
            } else {
                // Reuse line memories if vertical stride allows
                for (int i = 0; i < M_SIZE; i++) {
                    #pragma HLS UNROLL
                    if (config.stride < config.kernel_h) {
                        reuse_mode[i] = true;
                    } else {
                        reuse_mode[i] = false;
                    }
                }
                
                current_state = KPC_COMPUTE;
            }
            break;
        }
            
        case KPC_DONE:
            // Layer computation complete
            layer_done = true;
            
            for (int i = 0; i < M_SIZE; i++) {
                #pragma HLS UNROLL
                read_enable[i] = false;
                write_enable[i] = false;
            }
            break;
    }
}

/******************************************************************************
 * STANDALONE KPC FUNCTION
 ******************************************************************************/

void kpc_controller(
    LayerConfig config,
    bool start,
    bool stride_requests[M_SIZE][N_SIZE],
    ap_uint<5> line_selection[M_SIZE][N_SIZE],
    bool read_enable[M_SIZE],
    bool write_enable[M_SIZE],
    bool reuse_mode[M_SIZE],
    addr_t ra_r[M_SIZE],
    addr_t ra_n[M_SIZE],
    bool &next_stride,
    bool &compute_enable,
    bool &done
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    #pragma HLS ARRAY_PARTITION variable=stride_requests complete dim=0
    #pragma HLS ARRAY_PARTITION variable=line_selection complete dim=0
    #pragma HLS ARRAY_PARTITION variable=read_enable complete
    #pragma HLS ARRAY_PARTITION variable=write_enable complete
    #pragma HLS ARRAY_PARTITION variable=reuse_mode complete
    #pragma HLS ARRAY_PARTITION variable=ra_r complete
    #pragma HLS ARRAY_PARTITION variable=ra_n complete
    
    static KPCController kpc;
    #pragma HLS RESET variable=kpc
    
    if (start) {
        kpc.configure(config);
    }
    
    kpc.control(
        config,
        stride_requests,
        line_selection,
        read_enable,
        write_enable,
        reuse_mode,
        ra_r,
        ra_n,
        next_stride,
        compute_enable,
        done
    );
}
