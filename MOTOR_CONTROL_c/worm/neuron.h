
#ifndef NEURON_H
#define NEURON_H

#include <stddef.h>

#define MAX_NEURONS 50
#define MAX_SYNAPSES 100

typedef enum {
    NEURON_TYPE_SENSORY,
    NEURON_TYPE_INTER,
    NEURON_TYPE_MOTOR
} NeuronType_t;

typedef struct {
    float activation;
    NeuronType_t type;
    char name[32];
} Neuron_t;

typedef struct {
    int from;
    int to;
    float weight;
} Synapse_t;

// These are declared extern to be defined in another file (neuron.c)
extern Neuron_t neurons[MAX_NEURONS];
extern Synapse_t synapses[MAX_SYNAPSES];
extern size_t neuron_count;
extern size_t synapse_count;

// These are declared extern to be defined in neural_init.c
extern const char* neuron_names[MAX_NEURONS];
extern Synapse_t neural_synapses[MAX_SYNAPSES];
extern const size_t NUM_NEURONS_INIT;
extern const size_t NUM_SYNAPSES_INIT;

// Function declarations
void NeuralNet_init(void);
void NeuralNet_step(float* sensory_input, float* motor_output);

#endif // NEURON_H
