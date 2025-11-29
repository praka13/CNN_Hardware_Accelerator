/******************************************************************************
 * @file cnn_inference_engine.cpp
 * @brief Top-level CNN Inference Engine implementation  
 * @description Complete system: IEC + KPU (PE Array + Line Memories) + CU
 ******************************************************************************/

#include "cnn_inference_engine.h"

/******************************************************************************
 * TOP-LEVEL CNN INFERENCE ENGINE IMPLEMENTATION
 ******************************************************************************/

void cnn_inference_engine(
    hls::stream<data_t> &input_stream,
    hls::stream<data_t> &weight_stream,
    hls::stream<data_t> &bias_stream,
    hls::stream<data_t> &output_stream,
    LayerConfig layer_configs[MAX_LAYERS],
    int num_layers,
    bool start,
    bool &done,
    bool &interrupt,
    int &class_number,
    int &current_layer,
    int &current_iteration,
    ap_uint<32> &total_cycles
) {
    // HLS Interface Pragmas
    #pragma HLS INTERFACE axis port=input_stream
    #pragma HLS INTERFACE axis port=weight_stream
    #pragma HLS INTERFACE axis port=bias_stream
    #pragma HLS INTERFACE axis port=output_stream
    
    #pragma HLS INTERFACE s_axilite port=layer_configs bundle=control
    #pragma HLS INTERFACE s_axilite port=num_layers bundle=control
    #pragma HLS INTERFACE s_axilite port=start bundle=control
    #pragma HLS INTERFACE s_axilite port=done bundle=control
    #pragma HLS INTERFACE s_axilite port=interrupt bundle=control
    #pragma HLS INTERFACE s_axilite port=class_number bundle=control
    #pragma HLS INTERFACE s_axilite port=current_layer bundle=control
    #pragma HLS INTERFACE s_axilite port=current_iteration bundle=control
    #pragma HLS INTERFACE s_axilite port=total_cycles bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    
    #pragma HLS DATAFLOW
    
    // =========================================================================
    // Internal Signals and Streams
    // =========================================================================
    
    // Streams between KPU and CU
    static hls::stream<data_t> kpu_to_cu_stream("kpu_to_cu");
    #pragma HLS STREAM variable=kpu_to_cu_stream depth=64
    
    static hls::stream<bool> kpu_valid_stream("kpu_valid");
    #pragma HLS STREAM variable=kpu_valid_stream depth=64
    
    // IEC control signals
    static bool kpu_start;
    static bool prefetch_active;
    static bool compute_active;
    static LayerConfig current_config;
    static bool iec_done;
    static bool iec_interrupt;
    static int final_class;
    static int layer_idx;
    static int iteration_idx;
    
    // KPU status signals
    static bool kpu_done;
    static ap_uint<32> kpu_cycles;
    
    // CU status signals
    static bool cu_classification_done;
    static int cu_class_number;
    static data_t cu_output_data;
    static bool cu_output_valid;
    
    // Cycle counter
    static ap_uint<32> cycle_counter = 0;
    #pragma HLS RESET variable=cycle_counter
    
    if (start) {
        cycle_counter = 0;
    }
    
    // =========================================================================
    // MODULE 1: Inference Engine Controller (IEC)
    // =========================================================================
    
    iec_controller(
        layer_configs,
        num_layers,
        start,
        kpu_done,
        cu_classification_done,
        cu_class_number,
        kpu_start,
        prefetch_active,
        compute_active,
        current_config,
        iec_done,
        iec_interrupt,
        final_class,
        layer_idx,
        iteration_idx
    );
    
    // =========================================================================
    // MODULE 2: Kernel Processing Unit (KPU) - PE Array + Line Memories
    // =========================================================================
    
    // Create intermediate stream for KPU output
    static hls::stream<data_t> kpu_output("kpu_output");
    #pragma HLS STREAM variable=kpu_output depth=128
    
    pe_array(
        input_stream,
        weight_stream,
        bias_stream,
        kpu_output,
        current_config,
        kpu_start,
        kpu_done,
        kpu_cycles
    );
    
    // Connect KPU output to CU input
    static data_t kpu_data;
    static bool kpu_data_valid = false;
    
    if (!kpu_output.empty()) {
        kpu_data = kpu_output.read();
        kpu_data_valid = true;
        
        // Write to stream for CU
        if (!kpu_to_cu_stream.full()) {
            kpu_to_cu_stream.write(kpu_data);
            kpu_valid_stream.write(true);
        }
    } else {
        kpu_data_valid = false;
    }
    
    // =========================================================================
    // MODULE 3: Classify Unit (CU) - DSR + CUC + CNG + ACSU
    // =========================================================================
    
    // Read from KPU stream
    static data_t cu_input_data;
    static bool cu_input_valid;
    
    if (!kpu_to_cu_stream.empty() && !kpu_valid_stream.empty()) {
        cu_input_data = kpu_to_cu_stream.read();
        cu_input_valid = kpu_valid_stream.read();
    } else {
        cu_input_data = 0;
        cu_input_valid = false;
    }
    
    classify_unit(
        cu_input_data,
        cu_input_valid,
        layer_idx,
        current_config,
        cu_output_data,
        cu_output_valid,
        cu_class_number,
        cu_classification_done
    );
    
    // =========================================================================
    // OUTPUT ROUTING
    // =========================================================================
    
    // Route output based on whether classification is active
    if (cu_classification_done) {
        // Classification layer: Output class number
        // The actual activation values can be optionally output as well
        // For now, we just signal completion via class_number
        class_number = cu_class_number;
    } else if (cu_output_valid) {
        // Normal layer: Output activations/partial sums
        if (!output_stream.full()) {
            output_stream.write(cu_output_data);
        }
    }
    
    // =========================================================================
    // STATUS OUTPUTS
    // =========================================================================
    
    done = iec_done;
    interrupt = iec_interrupt;
    current_layer = layer_idx;
    current_iteration = iteration_idx;
    
    // Update cycle counter
    if (!iec_done) {
        cycle_counter++;
    }
    total_cycles = cycle_counter;
}
