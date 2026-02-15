#ifndef NEURON_H
#define NEURON_H

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include "libttak/math/fx.h"
#include "libttak/mem/arena.h"

// Maximum numbers of neurons and synapses supported
#define MAX_NEURONS 300
#define MAX_SYNAPSES 10000

// Common neuron indices
#define SENSOR_NEURON_DIST_IDX 0
#define SENSOR_NEURON_HOST_L_IDX 1
#define SENSOR_NEURON_HOST_R_IDX 2
#define MOTOR_NEURON_L_IDX 78
#define MOTOR_NEURON_R_IDX 79

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
    ttak_fx_t activation_fx;
    ttak_fx_t previous_activation_fx;
    NeuronType_t type;
    char name[32];
} Neuron_t;

// Synapse (connection) data structure
typedef struct {
    int from;
    int to;
    ttak_fx_t weight_fx;
    ttak_fx_t synaptic_strength_fx;
    ttak_fx_t neurotransmitter_type_fx; // 1.0 Dopamine, -1.0 Serotonin
    ttak_fx_t eligibility_trace_fx;
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
extern float atp_level;

/*
 * Initializes the synapses with a biological structure
 */
void init_synapses_biological(void);

/*
 * Initializes neural network neurons and synapses
 */
void NeuralNet_init(ttak_arena_t* arena);

/*
 * Performs one neural network step:
 * - sensory_input: input array for sensory neurons
 * - motor_output: output array to be filled with motor neuron activations
 */
void NeuralNet_step(const float* sensory_input, float* motor_output);

/*
 * Loads the neural network state from a file.
 */
void NeuralNet_load(const char* filename, ttak_arena_t* arena);

/*
 * Saves the neural network state to a file.
 */
void NeuralNet_save(const char* filename);

#endif // NEURON_H
