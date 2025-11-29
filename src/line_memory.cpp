/******************************************************************************
 * @file line_memory.cpp
 * @brief Line Memory implementation
 * @description k×A memory with n parallel outputs, data reuse, and pre-fetch monitoring
 ******************************************************************************/

#include "line_memory.h"

/******************************************************************************
 * LINE MEMORY CLASS IMPLEMENTATION
 ******************************************************************************/

LineMemory::LineMemory() {
    #pragma HLS ARRAY_PARTITION variable=output_buffer complete
    #pragma HLS BIND_STORAGE variable=memory type=RAM_2P latency=1
    #pragma HLS RESOURCE variable=memory core=RAM_2P_BRAM
    
    write_ptr = 0;
    read_ptr_new = 0;
    read_ptr_reuse = 0;
    data_count = 0;
    ready_flag = false;
    
    // Initialize memory
    for (int i = 0; i < LINE_MEM_WIDTH; i++) {
        #pragma HLS UNROLL factor=4
        memory[i] = 0;
    }
    
    // Initialize output buffer
    for (int i = 0; i < N_SIZE; i++) {
        #pragma HLS UNROLL
        output_buffer[i] = 0;
    }
}

void LineMemory::write_data(data_t data_in, bool write_enable) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    
    if (write_enable) {
        memory[write_ptr] = data_in;
        write_ptr++;
        
        // Wrap around at end of line
        if (write_ptr >= LINE_MEM_WIDTH) {
            write_ptr = 0;
        }
        
        // Track data count for pre-fetch monitoring
        data_count++;
    }
}

void LineMemory::read_data(
    bool read_enable,
    bool reuse_mode,
    ap_uint<16> required_count,
    data_t outputs[N_SIZE],
    bool &ready
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    #pragma HLS ARRAY_PARTITION variable=outputs complete
    
    // Check if we have enough data for pre-fetch
    ready = (data_count >= required_count);
    
    if (read_enable) {
        // Select read pointer based on reuse mode
        addr_t read_ptr = reuse_mode ? read_ptr_reuse : read_ptr_new;
        
        // Read n consecutive values into output buffer
        for (int i = 0; i < N_SIZE; i++) {
            #pragma HLS UNROLL
            
            addr_t addr = read_ptr + i;
            if (addr >= LINE_MEM_WIDTH) {
                addr = addr - LINE_MEM_WIDTH;  // Wrap around
            }
            
            output_buffer[i] = memory[addr];
            outputs[i] = output_buffer[i];
        }
        
        // Update read pointer for next access
        if (!reuse_mode) {
            read_ptr_new += N_SIZE;  // Advance by n for new data
            if (read_ptr_new >= LINE_MEM_WIDTH) {
                read_ptr_new = 0;
            }
        }
    } else {
        // Output current buffer contents
        for (int i = 0; i < N_SIZE; i++) {
            #pragma HLS UNROLL
            outputs[i] = output_buffer[i];
        }
    }
}

void LineMemory::reset() {
    #pragma HLS INLINE
    
    write_ptr = 0;
    read_ptr_new = 0;
    read_ptr_reuse = 0;
    data_count = 0;
    ready_flag = false;
}

void LineMemory::set_read_pointers(addr_t ptr_new, addr_t ptr_reuse) {
    #pragma HLS INLINE
    
    read_ptr_new = ptr_new;
    read_ptr_reuse = ptr_reuse;
}

/******************************************************************************
 * STANDALONE LINE MEMORY FUNCTION
 ******************************************************************************/

void line_memory(
    data_t data_in,
    bool write_enable,
    bool read_enable,
    int write_selector,
    int read_selector,
    int reuse_selector,
    int ra_r,
    int ra_n,
    bool next_stride,
    int r,
    data_t O[N_SIZE],
    bool &ready
) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    #pragma HLS ARRAY_PARTITION variable=O complete
    
    // Static line memory instance
    static LineMemory lm;
    #pragma HLS RESET variable=lm
    
    // Write operation
    if (write_enable) {
        lm.write_data(data_in, true);
    }
    
    // Update read pointers
    lm.set_read_pointers(ra_n, ra_r);
    
    // Read operation
    bool reuse_mode = (reuse_selector == 1);
    lm.read_data(read_enable, reuse_mode, r, O, ready);
    
    // Handle stride transition
    if (next_stride) {
        // Advance pointers for next stride
        // This is controlled by KPC
    }
}

/******************************************************************************
 * ADDRESS GENERATION UNITS FOR LINE MEMORY
 ******************************************************************************/

// Write Address Generator (WAG)
class WriteAGU {
private:
    addr_t addr;
    
public:
    WriteAGU() : addr(0) {}
    
    addr_t get_address(bool increment, ap_uint<10> line_width) {
        #pragma HLS INLINE
        #pragma HLS PIPELINE II=1
        
        addr_t current = addr;
        
        if (increment) {
            addr++;
            if (addr >= line_width) {
                addr = 0;  // Wrap to start of line
            }
        }
        
        return current;
    }
    
    void reset() {
        #pragma HLS INLINE
        addr = 0;
    }
};

// Read Address Generator (RAG)
class ReadAGU {
private:
    addr_t addr_new;      // For new data
    addr_t addr_reuse;    // For reused data
    ap_uint<3> stride;
    
public:
    ReadAGU() : addr_new(0), addr_reuse(0), stride(1) {}
    
    void set_stride(ap_uint<3> s) {
        #pragma HLS INLINE
        stride = s;
    }
    
    addr_t get_new_address(bool increment, ap_uint<10> line_width) {
        #pragma HLS INLINE
        #pragma HLS PIPELINE II=1
        
        addr_t current = addr_new;
        
        if (increment) {
            addr_new += stride;
            if (addr_new >= line_width) {
                addr_new = 0;
            }
        }
        
        return current;
    }
    
    addr_t get_reuse_address(bool increment, ap_uint<10> line_width) {
        #pragma HLS INLINE
        #pragma HLS PIPELINE II=1
        
        addr_t current = addr_reuse;
        
        if (increment) {
            addr_reuse += stride;
            if (addr_reuse >= line_width) {
                addr_reuse = 0;
            }
        }
        
        return current;
    }
    
    void set_addresses(addr_t new_addr, addr_t reuse_addr) {
        #pragma HLS INLINE
        addr_new = new_addr;
        addr_reuse = reuse_addr;
    }
    
    void reset() {
        #pragma HLS INLINE
        addr_new = 0;
        addr_reuse = 0;
    }
};
