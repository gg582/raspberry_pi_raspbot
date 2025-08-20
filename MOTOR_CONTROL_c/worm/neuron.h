#ifndef NEURON_H
#define NEURON_H

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

// Maximum numbers of neurons and synapses supported
#define MAX_NEURONS 300
#define MAX_SYNAPSES 10000

// Enumeration for neuron types
typedef enum {
    NEURON_TYPE_SENSORY,
    NEURON_TYPE_EXCITATORY,
    NEURON_TYPE_INHIBITORY,
    NEURON_TYPE_MOTOR
} NeuronType_t;

// Enumeration for synapse types
typedef enum {
    SYNAPSE_TYPE_NORMAL,
    SYNAPSE_TYPE_ANTAGONISTIC
} SynapseType_t;

// Neuron data structure
typedef struct {
    float activation;
    float previous_activation; // Added for short-term memory
    NeuronType_t type;
    char name[32];
} Neuron_t;

// Synapse (connection) data structure
typedef struct {
    int from;
    int to;
    float weight;
    float synaptic_strength;
    float neurotransmitter_type; // 1.0 Dopamine, -1.0 Serotonin
    SynapseType_t type;
} Synapse_t;

// External declarations for initial neuron and synapse counts
extern const size_t NUM_NEURONS_INIT;
extern size_t NUM_SYNAPSES_INIT;

// Initial neuron names and synapse array declarations
extern const char* neuron_names[MAX_NEURONS];
extern Synapse_t neural_synapses[MAX_SYNAPSES];

// Global variables for learning and mood signals
extern float dopamine_level;
extern float serotonin_level;
extern float acetylcholine_level;
extern float gaba_level;
extern float epinephrine_level;
extern float norepinephrine_level;
extern float glutamate_level;
extern float endorphin_level;
extern float oxytocin_level;
extern float cortisol_level;
extern float stability_level; // NEW: Added for behavioral stability

/*
 * Initializes the synapses with a biological structure
 */
void init_synapses_biological(void);

/*
 * Initializes neural network neurons and synapses
 */
void NeuralNet_init(void);

/*
 * Performs one neural network step:
 * - sensory_input: input array for sensory neurons
 * - motor_output: output array to be filled with motor neuron activations
 */
void NeuralNet_step(float* sensory_input, float* motor_output);

/*
 * Loads the neural network state from a file.
 */
void NeuralNet_load(const char* filename);

/*
 * Saves the neural network state to a file.
 */
void NeuralNet_save(const char* filename);

#endif // NEURON_H
