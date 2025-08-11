#include "neuron.h"

/*
 * Perform one control step based on sensory input.
 * dist: distance measured by ultrasonic sensor (0..max)
 * motor_output: output array to be filled with motor neuron activations
 */
void control_step(float dist, float *motor_output) {
    float sensory_input[MAX_NEURONS] = {0};

    // Normalize distance sensor input:
    // The closer the obstacle, the higher the sensory input.
    // Assuming sensory neuron index 0 is the distance sensor.
    sensory_input[0] = 1.0f - dist;

    // Step the neural network using sensory input to produce motor outputs.
    NeuralNet_step(sensory_input, motor_output);
}

