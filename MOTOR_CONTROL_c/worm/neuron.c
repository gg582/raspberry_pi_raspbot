
#include "neuron.h"
#include <string.h>

// Global arrays for neurons and synapses
Neuron_t neurons[MAX_NEURONS];
Synapse_t synapses[MAX_SYNAPSES];
size_t neuron_count = 0;
size_t synapse_count = 0;

void NeuralNet_init(void) {
    neuron_count = 0;
    synapse_count = 0;

    // Initialize neurons based on the data from neural_init.c
    for (size_t i = 0; i < NUM_NEURONS_INIT; i++) {
        strncpy(neurons[i].name, neuron_names[i], sizeof(neurons[i].name) - 1);
        neurons[i].name[sizeof(neurons[i].name) - 1] = '\0';
        neurons[i].activation = 0.0f;

        // Assign types based on neuron names
        if (strcmp(neuron_names[i], "SR04_DIST") == 0) {
            neurons[i].type = NEURON_TYPE_SENSORY;
        } else if (strcmp(neuron_names[i], "MOTOR_LEFT") == 0 || strcmp(neuron_names[i], "MOTOR_RIGHT") == 0) {
            neurons[i].type = NEURON_TYPE_MOTOR;
        } else {
            neurons[i].type = NEURON_TYPE_INTER;
        }
        neuron_count++;
    }

    // Initialize synapses based on the data from neural_init.c
    for (size_t i = 0; i < NUM_SYNAPSES_INIT; i++) {
        synapses[i].from = neural_synapses[i].from;
        synapses[i].to = neural_synapses[i].to;
        synapses[i].weight = neural_synapses[i].weight;
        synapse_count++;
    }
}

void NeuralNet_step(float* sensory_input, float* motor_output) {
    // Reset motor neuron activations
    for (size_t i = 0; i < neuron_count; i++) {
        if (neurons[i].type == NEURON_TYPE_MOTOR) {
            neurons[i].activation = 0.0f;
        }
    }

    // Assign sensory inputs
    for (size_t i = 0; i < neuron_count; i++) {
        if (neurons[i].type == NEURON_TYPE_SENSORY) {
            // Assuming SENSOR_NEURON_DIST_IDX is 0
            neurons[i].activation = sensory_input[0];
        }
    }

    // Propagate activation through the network
    for (size_t i = 0; i < synapse_count; i++) {
        int from_idx = synapses[i].from;
        int to_idx = synapses[i].to;
        float weight = synapses[i].weight;
        neurons[to_idx].activation += neurons[from_idx].activation * weight;
    }

    // Copy motor neuron activations to the output array
    int motor_idx = 0;
    for (size_t i = 0; i < neuron_count; i++) {
        if (neurons[i].type == NEURON_TYPE_MOTOR) {
            motor_output[motor_idx++] = neurons[i].activation;
        }
    }
}
