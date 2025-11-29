/******************************************************************************
 * @file iec_controller.h
 * @brief Inference Engine Controller header
 * @description Top-level controller with AXI interfaces and layer scheduling
 ******************************************************************************/

#ifndef IEC_CONTROLLER_H
#define IEC_CONTROLLER_H

#include "../include/cnn_types.h"

/******************************************************************************
 * IEC CONTROLLER CLASS
 ******************************************************************************/

class IECController {
private:
    iec_state_t current_state;
    
    // Layer management
    int current_layer_idx;
    int total_layers;
    
    // Iteration management
    ap_uint<16> current_iteration;
    ap_uint<16> iterations_per_layer;
    
    // Data tracking for pre-fetch logic
    ap_uint<16> data_fetched;      // j in algorithm
    ap_uint<16> data_required;     // rl in algorithm
    
    // Classification tracking
    int classification_result;
    bool classification_complete;
    
public:
    IECController();
    
    // Main control function
    void control(
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
    );
    
    // Reset controller
    void reset();
};

/******************************************************************************
 * STANDALONE IEC FUNCTION
 ******************************************************************************/

void iec_controller(
    // Layer configurations
    LayerConfig layer_configs[MAX_LAYERS],
    int num_layers,
    
    // Control signals
    bool start,
    bool kpu_done,
    bool cu_classification_done,
    int cu_class_number,
    
    // Outputs to KPU
    bool &kpu_start,
    bool &prefetch_active,
    bool &compute_active,
    LayerConfig &current_config,
    
    // Status outputs
    bool &done,
    bool &interrupt,
    int &final_class,
    int &current_layer,
    int &current_iteration
);

#endif // IEC_CONTROLLER_H
