/******************************************************************************
 * @file cnn_inference_engine.h
 * @brief Top-level CNN Inference Engine header
 * @description Complete integration of IEC + KPU + CU
 ******************************************************************************/

#ifndef CNN_INFERENCE_ENGINE_H
#define CNN_INFERENCE_ENGINE_H

#include "../include/cnn_types.h"
#include "iec_controller.h"
#include "pe_array.h"
#include "classify_unit.h"

/******************************************************************************
 * TOP-LEVEL CNN INFERENCE ENGINE
 ******************************************************************************/

void cnn_inference_engine(
    // AXI4-Stream interfaces for data I/O
    hls::stream<data_t> &input_stream,
    hls::stream<data_t> &weight_stream,
    hls::stream<data_t> &bias_stream,
    hls::stream<data_t> &output_stream,
    
    // AXI4-Lite interface for configuration
    LayerConfig layer_configs[MAX_LAYERS],
    int num_layers,
    
    // Control signals
    bool start,
    bool &done,
    bool &interrupt,
    
    // Classification output
    int &class_number,
    
    // Status outputs
    int &current_layer,
    int &current_iteration,
    ap_uint<32> &total_cycles
);

#endif // CNN_INFERENCE_ENGINE_H
