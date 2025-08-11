#include "neuron.h"

/*
 * Definition of neuron names used in the network.
 * This array provides human-readable names for each neuron index.
 */
const char* neuron_names[MAX_NEURONS] = {
    "SR04_DIST",      // Sensory neuron for ultrasonic distance sensor
    "MOTOR_LEFT",    // Motor neuron controlling left wheel
    "MOTOR_RIGHT"    // Motor neuron controlling right wheel
};

/*
 * Definition of synapses between neurons.
 * Each synapse connects a "from" neuron to a "to" neuron with a given weight.
 */
Synapse_t neural_synapses[MAX_SYNAPSES] = {
    // Sensory neuron 0 (distance sensor) excites left motor neuron positively
    {0, 1, 1.0f}, 
    
    // Sensory neuron 0 inhibits right motor neuron (negative weight)
    {0, 2, -1.0f}  
};

/*
 * Initial counts for neurons and synapses in this network
 */
const size_t NUM_NEURONS_INIT = 3;
const size_t NUM_SYNAPSES_INIT = 2;

