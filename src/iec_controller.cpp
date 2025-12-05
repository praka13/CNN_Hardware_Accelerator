/******************************************************************************
 * @file iec_controller.cpp
 * @brief Inference Engine Controller implementation
 * @description Layer scheduling, pre-fetch control, iteration management
 ******************************************************************************/

#include "iec_controller.h"

/******************************************************************************
 * IEC CONTROLLER IMPLEMENTATION
 ******************************************************************************/

IECController::IECController() {
    current_state = IEC_IDLE;
    current_layer_idx = 0;
    total_layers = 0;
    current_iteration = 0;
    iterations_per_layer = 0;
    data_fetched = 0;
    data_required = 0;
    classification_result = -1;
    classification_complete = false;
}

void IECController::reset() {
    #pragma HLS INLINE
    
    current_state = IEC_IDLE;
    current_layer_idx = 0;
    current_iteration = 0;
    data_fetched = 0;
    classification_result = -1;
    classification_complete = false;
}

void IECController::control(
    LayerConfig layer_configs[MAX_LAYERS],
    int num_layers,
    bool start,
    bool kpu_done,
    bool cu_classification_done,
    int cu_class_number,
    bool &kpu_start,
    bool &prefetch_active,
    bool &compute_active,
    LayerConfig &current_config,
    bool &done,
    bool &interrupt,
    int &final_class,
    int &layer_out,
    int &iteration_out
) {
    #pragma HLS PIPELINE II=1
    #pragma HLS ARRAY_PARTITION variable=layer_configs cyclic factor=4
    
    // Default outputs
    kpu_start = false;
    prefetch_active = false;
    compute_active = false;
    done = false;
    interrupt = false;
    
    // FSM State Machine
    switch (current_state) {
        
        case IEC_IDLE:
            // Wait for start signal
            if (start) {
                current_state = IEC_CONFIG;
                current_layer_idx = 0;
                total_layers = num_layers;
                current_iteration = 1;  // i = 1 in algorithm
                data_fetched = 0;       // j = 0 in algorithm
            }
            break;
        
        case IEC_CONFIG:
            // Load configuration for current layer (l)
            current_config = layer_configs[current_layer_idx];
            
            // Set layer-specific parameters
            iterations_per_layer = current_config.nl;
            data_required = current_config.rl;
            data_fetched = 0;
            current_iteration = 1;
            
            // Move to pre-fetch state
            current_state = IEC_PREFETCH;
            break;
        
        case IEC_PREFETCH:
            // Pre-fetch rl data items (Step 3 in algorithm)
            prefetch_active = true;
            
            // Simulate data fetching (in actual implementation, DMA would fetch)
            data_fetched++;
            
            // Step 4: Wait until j >= rl
            if (data_fetched >= data_required) {
                // Enough data pre-fetched, start computation
                current_state = IEC_COMPUTE;
                kpu_start = true;
            }
            break;
        
        case IEC_COMPUTE: {
            // Step 5: Start processing while continuing to fetch remaining data
            compute_active = true;
            prefetch_active = true;  // Continue fetching while computing
            
            // Continue fetching data
            data_fetched++;
            
            // Step 6: If current fetch complete and not last iteration,
            // start pre-fetching for next iteration
            bool fetch_for_current_complete = (data_fetched >= current_config.input_h * current_config.input_w * current_config.input_c / iterations_per_layer);
            bool not_last_iteration = (current_iteration < iterations_per_layer);
            
            if (fetch_for_current_complete && not_last_iteration) {
                // Start pre-fetching for iteration i+1
                data_fetched = 0;
            }
            
            // Step 7: When processing complete
            if (kpu_done) {
                current_state = IEC_NEXT_ITER;
            }
            break;
        }
        
        case IEC_NEXT_ITER:
            // Check if we need to classify or output
            if (current_config.is_fc_last) {
                // FClast layer: Check if classification is done
                if (cu_classification_done) {
                    // Step 9: Output CN-DC (final classification result)
                    classification_result = cu_class_number;
                    classification_complete = true;
                    current_state = IEC_NEXT_LAYER;
                } else {
                    // Continue processing activations for classification
                    current_iteration++;
                    if (current_iteration <= iterations_per_layer) {
                        current_state = IEC_PREFETCH;
                    } else {
                        current_state = IEC_CLASSIFY;
                    }
                }
            } else {
                // Normal layer: Output activation or partial sum (implicit)
                current_iteration++;
                
                // Step 8: Check if all iterations complete
                if (current_iteration > iterations_per_layer) {
                    current_state = IEC_NEXT_LAYER;
                } else {
                    // More iterations for this layer
                    current_state = IEC_PREFETCH;
                    data_fetched = 0;
                }
            }
            break;
        
        case IEC_CLASSIFY:
            // Wait for classification to complete
            if (cu_classification_done) {
                classification_result = cu_class_number;
                classification_complete = true;
                current_state = IEC_NEXT_LAYER;
            }
            break;
        
        case IEC_NEXT_LAYER:
            // Move to next layer
            current_layer_idx++;
            
            // Check if all layers processed
            if (current_layer_idx >= total_layers) {
                current_state = IEC_DONE;
                interrupt = true;  // Signal processor
            } else {
                // Configure next layer
                current_state = IEC_CONFIG;
            }
            break;
        
        case IEC_DONE:
            // All layers complete
            done = true;
            final_class = classification_result;
            interrupt = true;
            
            // Output status
            layer_out = current_layer_idx;
            iteration_out = current_iteration;
            break;
    }
}

/******************************************************************************
 * STANDALONE IEC FUNCTION
 ******************************************************************************/

void iec_controller(
    LayerConfig layer_configs[MAX_LAYERS],
    int num_layers,
    bool start,
    bool kpu_done,
    bool cu_classification_done,
    int cu_class_number,
    bool &kpu_start,
    bool &prefetch_active,
    bool &compute_active,
    LayerConfig &current_config,
    bool &done,
    bool &interrupt,
    int &final_class,
    int &current_layer,
    int &current_iteration
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    #pragma HLS INTERFACE s_axilite port=layer_configs
    #pragma HLS INTERFACE s_axilite port=num_layers
    #pragma HLS INTERFACE s_axilite port=start
    #pragma HLS INTERFACE s_axilite port=return
    
    static IECController iec;
    #pragma HLS RESET variable=iec
    
    iec.control(
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
        done,
        interrupt,
        final_class,
        current_layer,
        current_iteration
    );
}
