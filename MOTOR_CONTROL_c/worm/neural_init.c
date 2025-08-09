
#include "neuron.h"

// Define the structure of the neural network
// This file is generated to contain all network-specific data.

// Neuron names for easy reference
const char* neuron_names[MAX_NEURONS] = {
    "SR04_DIST",      // Sensory neuron for ultrasonic distance
    "MOTOR_LEFT",    // Motor neuron for the left wheel
    "MOTOR_RIGHT"    // Motor neuron for the right wheel
};

// Synapses (connections) with their weights
Synapse_t neural_synapses[MAX_SYNAPSES] = {
    // A close object (high sensory input) causes forward motion.
    {0, 1, 1.0f}, // SR04_DIST -> MOTOR_LEFT
    {0, 2, -1.0f}  // SR04_DIST -> MOTOR_RIGHT
};

// Number of neurons and synapses
const size_t NUM_NEURONS_INIT = 3;
const size_t NUM_SYNAPSES_INIT = 2;
