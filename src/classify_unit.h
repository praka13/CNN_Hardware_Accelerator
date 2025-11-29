/******************************************************************************
 * @file classify_unit.h
 * @brief Classify Unit header
 * @description Hardware-efficient classification with DSR, CUC, CNG, ACSU
 ******************************************************************************/

#ifndef CLASSIFY_UNIT_H
#define CLASSIFY_UNIT_H

#include "../include/cnn_types.h"

/******************************************************************************
 * DATA & SIGNAL ROUTER (DSR)
 ******************************************************************************/

void dsr(
    data_t ac_psum_in,          // Activation/partial sum input from KPU
    bool valid_in,              // Valid signal
    int current_layer,          // Current layer number
    int fc_last_layer,          // FClast layer number
    data_t &ac_to_acsu,        // Activation to ACSU
    data_t &ac_to_output,      // Activation to output
    bool &valid_to_acsu,        // Valid to ACSU
    bool &valid_to_output       // Valid to output
);

/******************************************************************************
 * CLASSIFY UNIT CONTROLLER (CUC)
 ******************************************************************************/

void cuc(
    bool valid_in,              // Valid activation input
    int current_layer,          // Current layer number
    int fc_last_layer,          // FClast layer number
    int num_classes,            // Total number of classes
    int &current_class_count,   // Current class counter
    bool &cng_enable,           // Enable CNG
    bool &acsu_enable,          // Enable ACSU
    bool &reset,                // Reset signal
    bool &classification_done   // Classification complete
);

/******************************************************************************
 * CLASS NUMBER GENERATOR (CNG)
 ******************************************************************************/

void cng(
    bool enable,                // Enable counting
    bool reset,                 // Reset to 0
    int &class_number           // Output class number (0 to N-1)
);

/******************************************************************************
 * ACTIVATION SEARCHING UNIT (ACSU)
 ******************************************************************************/

void acsu(
    data_t ac_in,               // Activation input (ACi)
    int class_number_in,        // Class number input (CNi)
    bool enable,                // Enable comparison
    bool reset,                 // Reset to initial state
    data_t &ac_max,            // Current maximum activation
    int &class_number_out       // Class number of maximum (CN-DC)
);

/******************************************************************************
 * COMPLETE CLASSIFY UNIT
 ******************************************************************************/

void classify_unit(
    data_t ac_psum_in,          // Input from KPU
    bool valid_in,
    int current_layer,
    LayerConfig config,
    data_t &output_data,
    bool &output_valid,
    int &final_class_number,
    bool &classification_done
);

#endif // CLASSIFY_UNIT_H
