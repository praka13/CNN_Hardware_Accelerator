/******************************************************************************
 * @file pe_array.cpp
 * @brief PE Array with Line Memories implementation
 * @description Complete KPU: m×n PEs + m line memories + KPC controller
 ******************************************************************************/

#include "pe_array.h"

/******************************************************************************
 * PE ARRAY IMPLEMENTATION
 ******************************************************************************/

void pe_array(
    hls::stream<data_t> &input_stream,
    hls::stream<data_t> &weight_stream,
    hls::stream<data_t> &bias_stream,
    hls::stream<data_t> &output_stream,
    LayerConfig config,
    bool start,
    bool &done,
    ap_uint<32> &cycle_count
) {
    #pragma HLS INLINE off
    #pragma HLS DATAFLOW
    
    // PE instances (m×n array)
    static data_t pe_outputs[M_SIZE][N_SIZE];
    #pragma HLS ARRAY_PARTITION variable=pe_outputs complete dim=0
    
    static bool pe_valid[M_SIZE][N_SIZE];
    #pragma HLS ARRAY_PARTITION variable=pe_valid complete dim=0
    
    static bool pe_stride_req[M_SIZE][N_SIZE];
    #pragma HLS ARRAY_PARTITION variable=pe_stride_req complete dim=0
    
    // Line memory instances (m line memories)
    static data_t line_outputs[M_SIZE][N_SIZE];
    #pragma HLS ARRAY_PARTITION variable=line_outputs complete dim=0
    
    static bool line_ready[M_SIZE];
    #pragma HLS ARRAY_PARTITION variable=line_ready complete
    
    // Control signals from KPC
    static ap_uint<5> line_selection[M_SIZE][N_SIZE];
    #pragma HLS ARRAY_PARTITION variable=line_selection complete dim=0
    
    static bool read_enable[M_SIZE];
    #pragma HLS ARRAY_PARTITION variable=read_enable complete
    
    static bool write_enable[M_SIZE];
    #pragma HLS ARRAY_PARTITION variable=write_enable complete
    
    static bool reuse_mode[M_SIZE];
    #pragma HLS ARRAY_PARTITION variable=reuse_mode complete
    
    static addr_t ra_r[M_SIZE];
    #pragma HLS ARRAY_PARTITION variable=ra_r complete
    
    static addr_t ra_n[M_SIZE];
    #pragma HLS ARRAY_PARTITION variable=ra_n complete
    
    static bool next_stride;
    static bool compute_enable;
    static bool kpc_done;
    
    // Cycle counter
    static ap_uint<32> cycles = 0;
    #pragma HLS RESET variable=cycles
    
    if (start) {
        cycles = 0;
    }
    
    // =========================================================================
    // STEP 1: Input Distribution to Line Memories
    // =========================================================================
    
    // Read input data and distribute to line memories
    if (!input_stream.empty()) {
        data_t input_data = input_stream.read();
        
        // Distribute input to appropriate line memory based on write_enable
        // In actual implementation, routing logic would direct to specific line memory
        // For simplicity, we use round-robin or sequential distribution
        static ap_uint<5> write_line_idx = 0;
        
        for (int i = 0; i < M_SIZE; i++) {
            #pragma HLS UNROLL
            
            if (write_enable[i] && i == write_line_idx) {
                // Write to this line memory
                line_memory(
                    input_data,
                    true,  // write_enable
                    false, // read_enable
                    0, 0, 0,
                    ra_r[i], ra_n[i],
                    next_stride,
                    config.rl,
                    line_outputs[i],
                    line_ready[i]
                );
            }
        }
        
        write_line_idx++;
        if (write_line_idx >= M_SIZE) {
            write_line_idx = 0;
        }
    }
    
    // =========================================================================
    // STEP 2: Line Memory Read Operations
    // =========================================================================
    
    for (int i = 0; i < M_SIZE; i++) {
        #pragma HLS UNROLL
        
        if (read_enable[i]) {
            // Read n outputs from this line memory
            line_memory(
                0,            // data_in (not used for read)
                false,        // write_enable
                true,         // read_enable
                0, 0,
                reuse_mode[i] ? 1 : 0,
                ra_r[i], ra_n[i],
                next_stride,
                config.rl,
                line_outputs[i],
                line_ready[i]
            );
        }
    }
    
    // =========================================================================
    // STEP 3: PE Array Computation
    // =========================================================================
    
    // Process with PE array
    for (int i = 0; i < M_SIZE; i++) {
        #pragma HLS UNROLL
        
        for (int j = 0; j < N_SIZE; j++) {
            #pragma HLS UNROLL
            
            // Gather inputs for this PE from all m line memories
            data_t pe_inputs[M_SIZE];
            #pragma HLS ARRAY_PARTITION variable=pe_inputs complete
            
            for (int k = 0; k < M_SIZE; k++) {
                #pragma HLS UNROLL
                pe_inputs[k] = line_outputs[k][j];
            }
            
            // Read weight from stream (if available)
            data_t weight = 0;
            if (!weight_stream.empty()) {
                weight = weight_stream.read();
            }
            
            // Read bias from stream (if available)
            data_t bias = 0;
            if (!bias_stream.empty()) {
                bias = bias_stream.read();
            }
            
            // Execute PE
            pe_unit(
                pe_inputs,
                weight,
                bias,
                line_selection[i][j],
                (config.layer_type == CONV || config.layer_type == FC), // mac_max_mode
                false,  // sign_override (handle separately for first layer)
                compute_enable,
                (start && cycles == 0),  // reset on start
                pe_outputs[i][j],
                pe_stride_req[i][j],
                pe_valid[i][j]
            );
        }
    }
    
    // =========================================================================
    // STEP 4: Kernel Processing Controller
    // =========================================================================
    
    kpc_controller(
        config,
        start,
        pe_stride_req,
        line_selection,
        read_enable,
        write_enable,
        reuse_mode,
        ra_r,
        ra_n,
        next_stride,
        compute_enable,
        kpc_done
    );
    
    // =========================================================================
    // STEP 5: Output Collection
    // =========================================================================
    
    // Collect valid outputs from PEs and write to output stream
    for (int i = 0; i < M_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        
        for (int j = 0; j < N_SIZE; j++) {
            #pragma HLS UNROLL
            
            if (pe_valid[i][j]) {
                // Apply activation based on layer type
                data_t activated_output;
                
                switch (config.layer_type) {
                    case RELU:
                        activated_output = relu_with_szd(pe_outputs[i][j]);
                        break;
                    
                    case RELU6:
                        activated_output = relu6_with_szd(pe_outputs[i][j]);
                        break;
                    
                    case MAXPOOL:
                    case AVGPOOL:
                    case CONV:
                    case FC:
                    default:
                        activated_output = pe_outputs[i][j];
                        break;
                }
                
                // Write to output stream
                if (!output_stream.full()) {
                    output_stream.write(activated_output);
                }
            }
        }
    }
    
    // Update cycle count
    if (!kpc_done) {
        cycles++;
    }
    
    cycle_count = cycles;
    done = kpc_done;
}
