/******************************************************************************
 * @file kpc_controller.h
 * @brief Kernel Processing Controller (KPC) header
 * @description FSM-based controller for PE array and line memory coordination
 ******************************************************************************/

#ifndef KPC_CONTROLLER_H
#define KPC_CONTROLLER_H

#include "../include/cnn_types.h"

/******************************************************************************
 * KPC CONTROLLER CLASS
 ******************************************************************************/

class KPCController {
private:
    kpc_state_t current_state;
    
    // Current position in feature map
    ap_uint<10> current_row;
    ap_uint<10> current_col;
    
    // Iteration counters
    ap_uint<16> iteration_count;
    ap_uint<16> total_iterations;
    
    // Data tracking
    ap_uint<16> data_fetched;
    ap_uint<16> data_required;
    
    // Stride control
    ap_uint<10> h_stride_count;
    ap_uint<10> v_stride_count;
    
public:
    KPCController();
    
    // Main control function
    void control(
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
    );
    
    // Reset controller
    void reset();
    
    // Configure for new layer
    void configure(LayerConfig &config);
};

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
);

#endif // KPC_CONTROLLER_H
