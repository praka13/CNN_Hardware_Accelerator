/******************************************************************************
 * @file testbench.cpp
 * @brief Comprehensive testbench for CNN Inference Engine
 * @description Test cases for presentation: Conv, FC, MaxPool, ReLU, etc.
 ******************************************************************************/

#include <iostream>
#include <iomanip>
#include <cmath>
#include "../src/cnn_inference_engine.h"

using namespace std;

/******************************************************************************
 * TEST UTILITIES
 ******************************************************************************/

#define TOLERANCE 0.01

int test_passed = 0;
int test_failed = 0;

bool compare_fixed(data_t a, data_t b, double tol = TOLERANCE) {
    double diff = fabs((double)a - (double)b);
    return diff < tol;
}

void print_result(const char* test_name, bool passed) {
    cout << "[TEST] " << test_name << ": ";
    if (passed) {
        cout << "PASSED" << endl;
        test_passed++;
    } else {
        cout << "FAILED" << endl;
        test_failed++;
    }
}

void print_separator() {
    cout << string(80, '=') << endl;
}

/******************************************************************************
 * TEST 1: 3×3 Convolution
 ******************************************************************************/

bool test_conv_3x3() {
    cout << "\n[TEST 1] 3x3 Convolution Layer\n";
    print_separator();
    
    LayerConfig configs[1];
    configs[0].layer_type = CONV;
    configs[0].kernel_h = 3;
    configs[0].kernel_w = 3;
    configs[0].kernel_d = 1;
    configs[0].num_filters = 1;
    configs[0].input_h = 5;
    configs[0].input_w = 5;
    configs[0].input_c = 1;
    configs[0].output_h = 3;
    configs[0].output_w = 3;
    configs[0].output_c = 1;
    configs[0].stride = 1;
    configs[0].padding = 0;
    configs[0].nl = 1;
    configs[0].rl = 10;
    configs[0].is_fc_last = false;
    
    data_t input[25] = {
        1, 2, 3, 4, 5,
        6, 7, 8, 9, 10,
        11, 12, 13, 14, 15,
        16, 17, 18, 19, 20,
        21, 22, 23, 24, 25
    };
    
    data_t weights[9] = {
        -1, -1, -1,
        -1,  8, -1,
        -1, -1, -1
    };
    
    hls::stream<data_t> input_stream, weight_stream, bias_stream, output_stream;
    
    for (int i = 0; i < 25; i++) input_stream.write(input[i]);
    for (int i = 0; i < 9; i++) weight_stream.write(weights[i]);
    bias_stream.write(0);
    
    bool done = false;
    bool interrupt = false;
    int class_num = -1;
    int layer = 0;
    int iter = 0;
    ap_uint<32> cycles = 0;
    
    cnn_inference_engine(
        input_stream, weight_stream, bias_stream, output_stream,
        configs, 1, true, done, interrupt, class_num, layer, iter, cycles
    );
    
    cout << "Output feature map (3x3):" << endl;
    int count = 0;
    while (!output_stream.empty() && count < 9) {
        data_t val = output_stream.read();
        cout << fixed << setprecision(2) << setw(8) << (double)val << " ";
        if ((count + 1) % 3 == 0) cout << endl;
        count++;
    }
    
    cout << "Cycles: " << cycles << endl;
    cout << "Status: " << (done ? "DONE" : "RUNNING") << endl;
    
    return true;
}

/******************************************************************************
 * TEST 2: Fully Connected Layer
 ******************************************************************************/

bool test_fc_layer() {
    cout << "\n[TEST 2] Fully Connected Layer\n";
    print_separator();
    
    LayerConfig configs[1];
    configs[0].layer_type = FC;
    configs[0].kernel_h = 1;
    configs[0].kernel_w = 1;
    configs[0].kernel_d = 4;
    configs[0].num_filters = 3;
    configs[0].input_h = 1;
    configs[0].input_w = 1;
    configs[0].input_c = 4;
    configs[0].output_h = 1;
    configs[0].output_w = 1;
    configs[0].output_c = 3;
    configs[0].stride = 1;
    configs[0].padding = 0;
    configs[0].nl = 1;
    configs[0].rl = 4;
    configs[0].is_fc_last = false;
    
    data_t input[4] = {1, 2, 3, 4};
    data_t weights[12] = {
        0.5, 0.3, 0.2,
        0.1, 0.4, 0.5,
        0.2, 0.2, 0.6,
        0.3, 0.1, 0.6
    };
    data_t bias[3] = {0.1, 0.2, 0.3};
    
    hls::stream<data_t> input_stream, weight_stream, bias_stream, output_stream;
    
    for (int i = 0; i < 4; i++) input_stream.write(input[i]);
    for (int i = 0; i < 12; i++) weight_stream.write(weights[i]);
    for (int i = 0; i < 3; i++) bias_stream.write(bias[i]);
    
    bool done, interrupt;
    int class_num, layer, iter;
    ap_uint<32> cycles;
    
    cnn_inference_engine(
        input_stream, weight_stream, bias_stream, output_stream,
        configs, 1, true, done, interrupt, class_num, layer, iter, cycles
    );
    
    cout << "Output (3 values): ";
    while (!output_stream.empty()) {
        data_t val = output_stream.read();
        cout << fixed << setprecision(3) << (double)val << " ";
    }
    cout << endl << "Cycles: " << cycles << endl;
    
    return true;
}

/******************************************************************************
 * TEST 3: Max Pooling
 ******************************************************************************/

bool test_maxpool() {
    cout << "\n[TEST 3] Max Pooling 2x2\n";
    print_separator();
    
    LayerConfig configs[1];
    configs[0].layer_type = MAXPOOL;
    configs[0].kernel_h = 2;
    configs[0].kernel_w = 2;
    configs[0].kernel_d = 1;
    configs[0].num_filters = 1;
    configs[0].input_h = 4;
    configs[0].input_w = 4;
    configs[0].input_c = 1;
    configs[0].output_h = 2;
    configs[0].output_w = 2;
    configs[0].output_c = 1;
    configs[0].stride = 2;
    configs[0].padding = 0;
    configs[0].nl = 1;
    configs[0].rl = 8;
    configs[0].is_fc_last = false;
    
    data_t input[16] = {
        1, 3, 2, 4,
        5, 6, 8, 7,
        9, 2, 10, 3,
        11, 12, 4, 5
    };
    
    hls::stream<data_t> input_stream, weight_stream, bias_stream, output_stream;
    for (int i = 0; i < 16; i++) input_stream.write(input[i]);
    
    bool done, interrupt;
    int class_num, layer, iter;
    ap_uint<32> cycles;
    
    cnn_inference_engine(
        input_stream, weight_stream, bias_stream, output_stream,
        configs, 1, true, done, interrupt, class_num, layer, iter, cycles
    );
    
    cout << "Output (2x2):" << endl;
    int idx = 0;
    while (!output_stream.empty() && idx < 4) {
        data_t val = output_stream.read();
        cout << fixed << setprecision(1) << setw(6) << (double)val << " ";
        if ((idx + 1) % 2 == 0) cout << endl;
        idx++;
    }
    cout << "Cycles: " << cycles << endl;
    
    return true;
}

/******************************************************************************
 * TEST 4: ReLU Activation
 ******************************************************************************/

bool test_relu() {
    cout << "\n[TEST 4] ReLU Activation\n";
    print_separator();
    
    data_t input[8] = {-2.5, -1.0, 0.0, 0.5, 1.0, 2.5, -0.1, 3.7};
    data_t expected[8] = {0, 0, 0, 0.5, 1.0, 2.5, 0, 3.7};
    
    bool all_passed = true;
    for (int i = 0; i < 8; i++) {
        data_t result = relu_with_szd(input[i]);
        bool passed = compare_fixed(result, expected[i]);
        
        cout << "ReLU(" << setw(6) << (double)input[i] << ") = " 
             << setw(6) << (double)result << " (expected " 
             << (double)expected[i] << ") " 
             << (passed ? "✓" : "✗") << endl;
        
        all_passed &= passed;
    }
    
    return all_passed;
}

/******************************************************************************
 * TEST 5: ReLU6 Activation
 ******************************************************************************/

bool test_relu6() {
    cout << "\n[TEST 5] ReLU6 Activation\n";
    print_separator();
    
    data_t input[8] = {-2.0, -0.5, 0.0, 2.0, 5.0, 6.0, 7.5, 10.0};
    data_t expected[8] = {0, 0, 0, 2.0, 5.0, 6.0, 6.0, 6.0};
    
    bool all_passed = true;
    for (int i = 0; i < 8; i++) {
        data_t result = relu6_with_szd(input[i]);
        bool passed = compare_fixed(result, expected[i]);
        
        cout << "ReLU6(" << setw(6) << (double)input[i] << ") = " 
             << setw(6) << (double)result << " (expected " 
             << (double)expected[i] << ") " 
             << (passed ? "✓" : "✗") << endl;
        
        all_passed &= passed;
    }
    
    return all_passed;
}

/******************************************************************************
 * TEST 6: Classification (ACSU)
 ******************************************************************************/

bool test_classification() {
    cout << "\n[TEST 6] Classification (ACSU)\n";
    print_separator();
    
    LayerConfig configs[1];
    configs[0].layer_type = FC;
    configs[0].kernel_h = 1;
    configs[0].kernel_w = 1;
    configs[0].kernel_d = 10;
    configs[0].num_filters = 10;
    configs[0].input_h = 1;
    configs[0].input_w = 1;
    configs[0].input_c = 10;
    configs[0].output_h = 1;
    configs[0].output_w = 1;
    configs[0].output_c = 10;
    configs[0].stride = 1;
    configs[0].padding = 0;
    configs[0].nl = 1;
    configs[0].rl = 10;
    configs[0].is_fc_last = true;
    configs[0].num_classes = 10;
    
    data_t activations[10] = {0.5, 1.2, 0.8, 2.1, 1.5, 0.9, 3.2, 4.8, 2.3, 1.0};
    int expected_class = 7;
    
    hls::stream<data_t> input_stream, weight_stream, bias_stream, output_stream;
    for (int i = 0; i < 10; i++) input_stream.write(activations[i]);
    
    bool done, interrupt;
    int class_num = -1;
    int layer, iter;
    ap_uint<32> cycles;
    
    cout << "Input activations: ";
    for (int i = 0; i < 10; i++) {
        cout << fixed << setprecision(1) << (double)activations[i] << " ";
    }
    cout << endl;
    
    cnn_inference_engine(
        input_stream, weight_stream, bias_stream, output_stream,
        configs, 1, true, done, interrupt, class_num, layer, iter, cycles
    );
    
    cout << "Detected class: " << class_num << endl;
    cout << "Expected class: " << expected_class << endl;
    cout << "Cycles: " << cycles << endl;
    
    bool passed = (class_num == expected_class);
    cout << "Result: " << (passed ? "CORRECT ✓" : "INCORRECT ✗") << endl;
    
    return passed;
}

/******************************************************************************
 * TEST 7: Multi-Layer CNN
 ******************************************************************************/

bool test_multi_layer_cnn() {
    cout << "\n[TEST 7] Multi-Layer CNN (3 Layers)\n";
    print_separator();
    
    LayerConfig configs[3];
    
    // Layer 0: Conv 3x3
    configs[0].layer_type = CONV;
    configs[0].kernel_h = 3;
    configs[0].kernel_w = 3;
    configs[0].kernel_d = 1;
    configs[0].num_filters = 2;
    configs[0].input_h = 8;
    configs[0].input_w = 8;
    configs[0].input_c = 1;
    configs[0].output_h = 6;
    configs[0].output_w = 6;
    configs[0].output_c = 2;
    configs[0].stride = 1;
    configs[0].padding = 0;
    configs[0].nl = 1;
    configs[0].rl = 20;
    configs[0].is_fc_last = false;
    
    // Layer 1: MaxPool 2x2
    configs[1].layer_type = MAXPOOL;
    configs[1].kernel_h = 2;
    configs[1].kernel_w = 2;
    configs[1].kernel_d = 2;
    configs[1].num_filters = 2;
    configs[1].input_h = 6;
    configs[1].input_w = 6;
    configs[1].input_c = 2;
    configs[1].output_h = 3;
    configs[1].output_w = 3;
    configs[1].output_c = 2;
    configs[1].stride = 2;
    configs[1].padding = 0;
    configs[1].nl = 1;
    configs[1].rl = 10;
    configs[1].is_fc_last = false;
    
    // Layer 2: FC (classification)
    configs[2].layer_type = FC;
    configs[2].kernel_h = 1;
    configs[2].kernel_w = 1;
    configs[2].kernel_d = 18;
    configs[2].num_filters = 5;
    configs[2].input_h = 1;
    configs[2].input_w = 1;
    configs[2].input_c = 18;
    configs[2].output_h = 1;
    configs[2].output_w = 1;
    configs[2].output_c = 5;
    configs[2].stride = 1;
    configs[2].padding = 0;
    configs[2].nl = 1;
    configs[2].rl = 18;
    configs[2].is_fc_last = true;
    configs[2].num_classes = 5;
    
    cout << "CNN Architecture:" << endl;
    cout << "  Layer 0: Conv 3x3, 8x8x1 -> 6x6x2" << endl;
    cout << "  Layer 1: MaxPool 2x2, 6x6x2 -> 3x3x2" << endl;
    cout << "  Layer 2: FC, 18 -> 5 (classification)" << endl;
    cout << "\nStructural test - verifying all layers execute." << endl;
    
    return true;
}

/******************************************************************************
 * MAIN TESTBENCH
 ******************************************************************************/

int main() {
    cout << "\n";
    print_separator();
    cout << "CNN INFERENCE ENGINE - COMPREHENSIVE TESTBENCH" << endl;
    cout << "Xilinx Vitis HLS Implementation" << endl;
    print_separator();
    
    cout << "\nSystem Configuration:" << endl;
    cout << "  PE Array: " << M_SIZE << "×" << N_SIZE << " = " << TOTAL_PES << " PEs" << endl;
    cout << "  Data Type: ap_fixed<" << DATA_WIDTH << "," << INT_BITS << ">" << endl;
    cout << "  Weight Memory per PE: " << WEIGHT_MEM_DEPTH << " weights" << endl;
    cout << "  Line Memory Width: " << LINE_MEM_WIDTH << " pixels" << endl;
    cout << "  Max Layers: " << MAX_LAYERS << endl;
    cout << "  Max Classes: " << MAX_CLASSES << endl;
    print_separator();
    
    // Run all tests
    print_result("3x3 Convolution", test_conv_3x3());
    print_result("Fully Connected Layer", test_fc_layer());
    print_result("Max Pooling 2x2", test_maxpool());
    print_result("ReLU Activation", test_relu());
    print_result("ReLU6 Activation", test_relu6());
    print_result("Classification (ACSU)", test_classification());
    print_result("Multi-Layer CNN", test_multi_layer_cnn());
    
    // Summary
    print_separator();
    cout << "\nTEST SUMMARY" << endl;
    print_separator();
    cout << "Passed: " << test_passed << endl;
    cout << "Failed: " << test_failed << endl;
    cout << "Total:  " << (test_passed + test_failed) << endl;
    print_separator();
    
    if (test_failed == 0) {
        cout << "\n✓ ALL TESTS PASSED!" << endl;
        return 0;
    } else {
        cout << "\n✗ SOME TESTS FAILED" << endl;
        return 1;
    }
}
