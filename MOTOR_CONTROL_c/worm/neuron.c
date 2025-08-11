#include "neuron.h"
#include <string.h>

/*
 * Global arrays to hold neurons and synapses
 */
Neuron_t neurons[MAX_NEURONS];
Synapse_t synapses[MAX_SYNAPSES];
size_t neuron_count = 0;
size_t synapse_count = 0;

/*
 * Initialize the neural network:
 * - Copy neuron names and initialize neuron properties (activation, type)
 * - Copy synapses and weights
 */
void NeuralNet_init(void) {
    neuron_count = 0;
    synapse_count = 0;

    // Initialize neurons from the names and assign types
    for (size_t i = 0; i < NUM_NEURONS_INIT; i++) {
        // Copy name safely
        strncpy(neurons[i].name, neuron_names[i], sizeof(neurons[i].name) - 1);
        neurons[i].name[sizeof(neurons[i].name) - 1] = '\0';

        // Initialize activation to zero
        neurons[i].activation = 0.0f;

        // Assign neuron type based on name
        if (strcmp(neuron_names[i], "SR04_DIST") == 0) {
            neurons[i].type = NEURON_TYPE_SENSORY;
        } else if (strcmp(neuron_names[i], "MOTOR_LEFT") == 0 || strcmp(neuron_names[i], "MOTOR_RIGHT") == 0) {
            neurons[i].type = NEURON_TYPE_MOTOR;
        } else {
            neurons[i].type = NEURON_TYPE_INTER;
        }
        neuron_count++;
    }

    // Initialize synapses based on neural_synapses data
    for (size_t i = 0; i < NUM_SYNAPSES_INIT; i++) {
        synapses[i].from = neural_synapses[i].from;
        synapses[i].to = neural_synapses[i].to;
        synapses[i].weight = neural_synapses[i].weight;
        synapse_count++;
    }
}

/*
 * Perform one step of neural network computation:
 * - Assign sensory inputs to sensory neurons
 * - Propagate activations through synapses to motor neurons
 * - Return motor neuron activations in motor_output array
 */
void NeuralNet_step(float* sensory_input, float* motor_output) {
    // Reset motor neuron activations to zero before computing
    for (size_t i = 0; i < neuron_count; i++) {
        if (neurons[i].type == NEURON_TYPE_MOTOR) {
            neurons[i].activation = 0.0f;
        }
    }

    // Assign sensory inputs to sensory neurons
    for (size_t i = 0; i < neuron_count; i++) {
        if (neurons[i].type == NEURON_TYPE_SENSORY) {
            // Currently assuming sensory neuron index 0 corresponds to sensory_input[0]
            neurons[i].activation = sensory_input[0];
        }
    }

    // Propagate activations through synapses
    for (size_t i = 0; i < synapse_count; i++) {
        int from_idx = synapses[i].from;
        int to_idx = synapses[i].to;
        float weight = synapses[i].weight;
        neurons[to_idx].activation += neurons[from_idx].activation * weight;
    }

    // Copy motor neuron activations to output array
    int motor_idx = 0;
    for (size_t i = 0; i < neuron_count; i++) {
        if (neurons[i].type == NEURON_TYPE_MOTOR) {
            motor_output[motor_idx++] = neurons[i].activation;
        }
    }
}

