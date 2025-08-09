#include "neuron.h"

void control_step(float dist, float *motor_output) {
    float sensory_input[MAX_NEURONS] = {0};

    // Example: normalize distance sensor (assuming sensor neuron index 0)
    sensory_input[0] = 1.0f - dist; // e.g. closer obstacle = higher input

    NeuralNet_step(sensory_input, motor_output);
}

