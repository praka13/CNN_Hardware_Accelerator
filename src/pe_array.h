/******************************************************************************
 * @file pe_array.h
 * @brief PE Array with Line Memories header
 * @description Instantiates m×n PE array and m line memories with interconnections
 ******************************************************************************/

#ifndef PE_ARRAY_H
#define PE_ARRAY_H

#include "../include/cnn_types.h"
#include "pe_unit.h"
#include "line_memory.h"
#include "kpc_controller.h"

/******************************************************************************
 * PE ARRAY FUNCTION
 ******************************************************************************/

void pe_array(
    // Input/output streams
    hls::stream<data_t> &input_stream,
    hls::stream<data_t> &weight_stream,
    hls::stream<data_t> &bias_stream,
    hls::stream<data_t> &output_stream,
    
    // Configuration
    LayerConfig config,
    
    // Control
    bool start,
    bool &done,
    
    // Status
    ap_uint<32> &cycle_count
);

#endif // PE_ARRAY_H
