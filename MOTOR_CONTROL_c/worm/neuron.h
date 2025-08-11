#ifndef NEURON_H
#define NEURON_H

#include <stddef.h>

// Maximum numbers of neurons and synapses supported
#define MAX_NEURONS 300
#define MAX_SYNAPSES 600

/*
 * Enumeration for neuron types:
 * Sensory neurons receive input from sensors,
 * Inter neurons are internal processing neurons,
 * Motor neurons control actuators.
 */
typedef enum {
    NEURON_TYPE_SENSORY,
    NEURON_TYPE_INTER,
    NEURON_TYPE_MOTOR
} NeuronType_t;

/*
 * Neuron data structure:
 * - activation: current activation value (float)
 * - type: neuron type (sensory, inter, motor)
 * - name: human-readable neuron identifier string
 */
typedef struct {
    float activation;
    NeuronType_t type;
    char name[32];
} Neuron_t;

/*
 * Synapse (connection) data structure:
 * - from: index of source neuron
 * - to: index of destination neuron
 * - weight: connection weight (float)
 */
typedef struct {
    int from;
    int to;
    float weight;
} Synapse_t;

// External declarations for initial neuron and synapse counts
extern const size_t NUM_NEURONS_INIT;
extern const size_t NUM_SYNAPSES_INIT;

// Initial neuron names and synapse array declarations
extern const char* neuron_names[MAX_NEURONS];
extern Synapse_t neural_synapses[MAX_SYNAPSES];

/*
 * Initialize neural network neurons and synapses
 */
void NeuralNet_init(void);

/*
 * Perform one neural network step:
 * - sensory_input: input array for sensory neurons
 * - motor_output: output array to be filled with motor neuron activations
 */
void NeuralNet_step(float* sensory_input, float* motor_output);

#endif // NEURON_H

