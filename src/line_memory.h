/******************************************************************************
 * @file line_memory.h
 * @brief Line Memory module header
 * @description Stores one row of feature map with n parallel outputs to PEs
 ******************************************************************************/

#ifndef LINE_MEMORY_H
#define LINE_MEMORY_H

#include "../include/cnn_types.h"

/******************************************************************************
 * LINE MEMORY CLASS
 ******************************************************************************/

class LineMemory {
private:
    // Main storage: k-bit × A memory (one row of feature map)
    data_t memory[LINE_MEM_WIDTH];
    
    // Output buffer: n registers for n parallel outputs
    data_t output_buffer[N_SIZE];
    
    // Address pointers
    addr_t write_ptr;
    addr_t read_ptr_new;      // For new data
    addr_t read_ptr_reuse;    // For reused data
    
    // Data count for pre-fetch monitoring
    ap_uint<16> data_count;
    
    // Ready signal
    bool ready_flag;
    
public:
    LineMemory();
    
    // Write operation
    void write_data(data_t data_in, bool write_enable);
    
    // Read operation with reuse logic
    void read_data(
        bool read_enable,
        bool reuse_mode,
        ap_uint<16> required_count,
        data_t outputs[N_SIZE],
        bool &ready
    );
    
    // Reset line memory
    void reset();
    
    // Set read pointers
    void set_read_pointers(addr_t ptr_new, addr_t ptr_reuse);
};

/******************************************************************************
 * STANDALONE LINE MEMORY FUNCTION
 ******************************************************************************/

void line_memory(
    data_t data_in,                 // Input data
    bool write_enable,              // Write enable
    bool read_enable,               // Read enable
    int write_selector,             // Write address selector
    int read_selector,              // Read address selector
    int reuse_selector,             // Reuse mode selector
    int ra_r,                       // Reuse address pointer
    int ra_n,                       // New address pointer
    bool next_stride,               // Next stride signal
    int r,                          // Minimum data count for ready
    data_t O[N_SIZE],              // n parallel outputs
    bool &ready                     // Ready signal (enough data)
);

#endif // LINE_MEMORY_H
