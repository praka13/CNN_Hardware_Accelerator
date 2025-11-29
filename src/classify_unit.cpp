/******************************************************************************
 * @file classify_unit.cpp
 * @brief Classify Unit implementation
 * @description Complete CU with DSR, CUC, CNG, and ACSU submodules
 ******************************************************************************/

#include "classify_unit.h"

/******************************************************************************
 * DATA & SIGNAL ROUTER (DSR) IMPLEMENTATION
 ******************************************************************************/

void dsr(
    data_t ac_psum_in,
    bool valid_in,
    int current_layer,
    int fc_last_layer,
    data_t &ac_to_acsu,
    data_t &ac_to_output,
    bool &valid_to_acsu,
    bool &valid_to_output
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    
    // Check if current layer is FClast
    bool is_fc_last = (current_layer == fc_last_layer);
    
    if (is_fc_last && valid_in) {
        // Route to ACSU for classification
        ac_to_acsu = ac_psum_in;
        valid_to_acsu = true;
        ac_to_output = 0;
        valid_to_output = false;
    } else {
        // Route to output (normal layer processing)
        ac_to_output = ac_psum_in;
        valid_to_output = valid_in;
        ac_to_acsu = 0;
        valid_to_acsu = false;
    }
}

/******************************************************************************
 * CLASSIFY UNIT CONTROLLER (CUC) IMPLEMENTATION
 ******************************************************************************/

void cuc(
    bool valid_in,
    int current_layer,
    int fc_last_layer,
    int num_classes,
    int &current_class_count,
    bool &cng_enable,
    bool &acsu_enable,
    bool &reset,
    bool &classification_done
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    
    static cuc_state_t state = CUC_IDLE;
    #pragma HLS RESET variable=state
    
    static int class_counter = 0;
    #pragma HLS RESET variable=class_counter
    
    // Default outputs
    cng_enable = false;
    acsu_enable = false;
    reset = false;
    classification_done = false;
    
    bool is_fc_last = (current_layer == fc_last_layer);
    
    switch (state) {
        case CUC_IDLE:
            if (is_fc_last && valid_in) {
                // Start classification
                state = CUC_ACTIVE;
                class_counter = 0;
                reset = true;  // Reset CNG and ACSU
            }
            break;
        
        case CUC_ACTIVE:
            if (valid_in) {
                // Enable CNG and ACSU
                cng_enable = true;
                acsu_enable = true;
                
                // Increment class counter
                class_counter++;
                current_class_count = class_counter;
                
                // Check if all classes processed
                if (class_counter >= num_classes) {
                    state = CUC_DONE;
                    classification_done = true;
                }
            }
            break;
        
        case CUC_DONE:
            classification_done = true;
            
            // Wait for next layer (reset when layer changes)
            if (!is_fc_last) {
                state = CUC_IDLE;
                class_counter = 0;
            }
            break;
    }
}

/******************************************************************************
 * CLASS NUMBER GENERATOR (CNG) IMPLEMENTATION
 ******************************************************************************/

void cng(
    bool enable,
    bool reset,
    int &class_number
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    
    static int counter = 0;
    #pragma HLS RESET variable=counter
    
    if (reset) {
        counter = 0;
        class_number = 0;
    } else if (enable) {
        // Output current counter value (CNi)
        class_number = counter;
        
        // Increment for next activation
        counter++;
    }
}

/******************************************************************************
 * ACTIVATION SEARCHING UNIT (ACSU) IMPLEMENTATION
 ******************************************************************************/

void acsu(
    data_t ac_in,
    int class_number_in,
    bool enable,
    bool reset,
    data_t &ac_max,
    int &class_number_out
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    
    // REG1: Stores ACMax (maximum activation seen so far)
    static data_t reg_ac_max = TO_FIXED(-1000);  // Initialize to very negative value
    #pragma HLS RESET variable=reg_ac_max
    
    // REG2: Stores CN-DC (class number of maximum activation)
    static int reg_class_num = 0;
    #pragma HLS RESET variable=reg_class_num
    
    if (reset) {
        // Reset registers
        reg_ac_max = TO_FIXED(-1000);
        reg_class_num = 0;
        ac_max = reg_ac_max;
        class_number_out = reg_class_num;
    } else if (enable) {
        // Comparator: Compare ACi with ACMax
        bool ac_is_greater = (ac_in > reg_ac_max);
        
        // MUX Selection Logic
        if (ac_is_greater) {
            // Update: ACi is new maximum
            reg_ac_max = ac_in;
            reg_class_num = class_number_in;
        }
        // Else: Keep previous values (no update needed)
        
        // Output current maximum and its class number
        ac_max = reg_ac_max;
        class_number_out = reg_class_num;
    } else {
        // Just output current values
        ac_max = reg_ac_max;
        class_number_out = reg_class_num;
    }
}

/******************************************************************************
 * COMPLETE CLASSIFY UNIT INTEGRATION
 ******************************************************************************/

void classify_unit(
    data_t ac_psum_in,
    bool valid_in,
    int current_layer,
    LayerConfig config,
    data_t &output_data,
    bool &output_valid,
    int &final_class_number,
    bool &classification_done
) {
    #pragma HLS INLINE off
    #pragma HLS DATAFLOW
    
    // Internal signals
    static data_t ac_to_acsu;
    static data_t ac_to_output;
    static bool valid_to_acsu;
    static bool valid_to_output;
    
    static int current_class_count;
    static bool cng_enable;
    static bool acsu_enable;
    static bool reset_signal;
    
    static int class_number_gen;
    static data_t ac_max_value;
    static int cn_dc;
    
    // Submodule 1: Data & Signal Router
    dsr(
        ac_psum_in,
        valid_in,
        current_layer,
        config.is_fc_last ? current_layer : (current_layer + 1), // FClast identifier
        ac_to_acsu,
        ac_to_output,
        valid_to_acsu,
        valid_to_output
    );
    
    // Submodule 2: Classify Unit Controller
    cuc(
        valid_to_acsu,
        current_layer,
        config.is_fc_last ? current_layer : (current_layer + 1),
        config.num_classes,
        current_class_count,
        cng_enable,
        acsu_enable,
        reset_signal,
        classification_done
    );
    
    // Submodule 3: Class Number Generator
    cng(
        cng_enable,
        reset_signal,
        class_number_gen
    );
    
    // Submodule 4: Activation Searching Unit
    acsu(
        ac_to_acsu,
        class_number_gen,
        acsu_enable,
        reset_signal,
        ac_max_value,
        cn_dc
    );
    
    // Output routing
    if (classification_done) {
        // Output final classification result
        output_data = ac_max_value;  // Maximum activation value
        output_valid = true;
        final_class_number = cn_dc;  // Class number with maximum activation
    } else {
        // Output normal layer activations/partial sums
        output_data = ac_to_output;
        output_valid = valid_to_output;
        final_class_number = -1;  // Invalid class number
    }
}
