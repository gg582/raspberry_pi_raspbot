#include "neuron.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Global variable declarations.
 */
Neuron_t neurons[MAX_NEURONS];
Synapse_t synapses[MAX_SYNAPSES];
size_t neuron_count = 0;
size_t synapse_count = 0;

// Neurotransmitter levels
float dopamine_level = 0.0f; // Represents RPE
float serotonin_level = 0.0f;
float acetylcholine_level = 0.0f;
float gaba_level = 0.0f;
float epinephrine_level = 0.0f;
float norepinephrine_level = 0.0f;
float glutamate_level = 0.0f;
float endorphin_level = 0.0f;
float oxytocin_level = 0.0f;
float cortisol_level = 0.0f;
float stability_level = 0.0f; // NEW

/*
 * Function definitions.
 */

float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

float swish_activation(float x) {
    return x * sigmoid(x);
}

float get_neuron_threshold(NeuronType_t type) {
    float base_threshold = 0.1f;
    float dynamic_threshold = base_threshold;
    
    // Serotonin, GABA, Endorphin, and Cortisol raise the threshold
    dynamic_threshold += serotonin_level * 0.1f;
    dynamic_threshold += gaba_level * 0.3f;
    dynamic_threshold += endorphin_level * 0.1f;
    dynamic_threshold += cortisol_level * 0.2f;
    
    // Dopamine, Acetylcholine, Norepinephrine, and Glutamate lower the threshold
    dynamic_threshold -= dopamine_level * 0.1f;
    dynamic_threshold -= acetylcholine_level * 0.2f;
    dynamic_threshold -= norepinephrine_level * 0.3f;
    dynamic_threshold -= glutamate_level * 0.1f;
    
    // A stable network is more decisive and has a lower threshold
    dynamic_threshold -= stability_level * 0.1f;
    
    return dynamic_threshold;
}

void apply_hebbian_rule(int from_idx, int to_idx) {
    if (dopamine_level > 0.0f &&
        neurons[from_idx].activation > get_neuron_threshold(neurons[from_idx].type) &&
        neurons[to_idx].activation > get_neuron_threshold(neurons[to_idx].type)) {
        for (size_t i = 0; i < synapse_count; i++) {
            if (synapses[i].from == from_idx && synapses[i].to == to_idx) {
                if (synapses[i].weight < 1.0f) {
                    synapses[i].weight += 0.2f * dopamine_level * glutamate_level;
                }
            }
        }
    } else if (dopamine_level < 0.0f) {
         for (size_t i = 0; i < synapse_count; i++) {
            if (synapses[i].from == from_idx && synapses[i].to == to_idx) {
                 synapses[i].weight *= (1.0f + 0.01f * dopamine_level);
            }
        }
    }
}

void NeuralNet_init(void) {
    neuron_count = NUM_NEURONS_INIT;
    init_synapses_biological();
    synapse_count = NUM_SYNAPSES_INIT;
    
    for (size_t i = 0; i < neuron_count; i++) {
        strncpy(neurons[i].name, neuron_names[i], sizeof(neurons[i].name) - 1);
        neurons[i].name[sizeof(neurons[i].name) - 1] = '\0';
        neurons[i].activation = 0.0f;
        neurons[i].previous_activation = 0.0f;
        
        if (strstr(neurons[i].name, "SR04_DIST") != NULL) {
            neurons[i].type = NEURON_TYPE_SENSORY;
        } else if (strstr(neurons[i].name, "MOTOR") != NULL) {
            neurons[i].type = NEURON_TYPE_MOTOR;
        } else if (strstr(neurons[i].name, "_EX_") != NULL) {
            neurons[i].type = NEURON_TYPE_EXCITATORY;
        } else if (strstr(neurons[i].name, "_IN_") != NULL) {
            neurons[i].type = NEURON_TYPE_INHIBITORY;
        }
    }
    
    for (size_t i = 0; i < synapse_count; i++) {
        synapses[i].synaptic_strength = 1.0f;
    }
}

void NeuralNet_step(float* sensory_input, float* motor_output) {
    for (size_t i = 0; i < neuron_count; i++) {
        neurons[i].previous_activation = neurons[i].activation;
    }

    for (size_t i = 3; i < neuron_count; i++) {
        neurons[i].activation = 0.0f;
    }

    neurons[0].activation = swish_activation(sensory_input[0]);
    neurons[1].activation = swish_activation(sensory_input[1]);
    neurons[2].activation = swish_activation(sensory_input[2]);

    for (size_t i = 0; i < synapse_count; i++) {
        int from = synapses[i].from;
        int to = synapses[i].to;
        float weight = synapses[i].weight;
        float strength = synapses[i].synaptic_strength;
        
        float input_signal = (neurons[from].activation * 0.7f + neurons[from].previous_activation * 0.3f) * weight * strength;

        if (input_signal > get_neuron_threshold(neurons[to].type)) {
            neurons[to].activation += input_signal;
        }

        if (neurons[from].activation > get_neuron_threshold(neurons[from].type)) {
            synapses[i].synaptic_strength *= 0.99;
        }
        if (synapses[i].synaptic_strength < 1.0) {
            synapses[i].synaptic_strength += 0.001;
        }
    }

    for (size_t i = 0; i < neuron_count; i++) {
        if (neurons[i].type == NEURON_TYPE_EXCITATORY || neurons[i].type == NEURON_TYPE_INHIBITORY) {
            neurons[i].activation = swish_activation(neurons[i].activation);
            for (size_t j = 0; j < synapse_count; j++) {
                if (synapses[j].to == i && (neurons[synapses[j].from].type == NEURON_TYPE_EXCITATORY || neurons[synapses[j].from].type == NEURON_TYPE_SENSORY)) {
                    apply_hebbian_rule(synapses[j].from, i);
                }
            }
        }
    }
    
    if (serotonin_level > 0.5) {
        for (size_t i = 0; i < synapse_count; i++) {
            if (synapses[i].neurotransmitter_type == -1.0) { 
                synapses[i].weight *= 0.99;
            }
        }
    }
    
    motor_output[0] = swish_activation(neurons[78].activation);
    motor_output[1] = swish_activation(neurons[79].activation);
}

void NeuralNet_load(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("No saved neural network found. Initializing a new one.\n");
        NeuralNet_init();
        return;
    }

    size_t file_neuron_count, file_synapse_count;
    fread(&file_neuron_count, sizeof(size_t), 1, file);
    fread(&file_synapse_count, sizeof(size_t), 1, file);

    if (file_neuron_count != NUM_NEURONS_INIT || file_synapse_count != NUM_SYNAPSES_INIT) {
        printf("Saved state mismatch. Initializing a new neural network.\n");
        fclose(file);
        NeuralNet_init();
        return;
    }

    neuron_count = file_neuron_count;
    synapse_count = file_synapse_count;

    fread(neurons, sizeof(Neuron_t), neuron_count, file);
    fread(synapses, sizeof(Synapse_t), synapse_count, file);

    fclose(file);
    printf("Neural network state loaded successfully from '%s'.\n", filename);
}

void NeuralNet_save(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Failed to save neural network");
        return;
    }

    fwrite(&neuron_count, sizeof(size_t), 1, file);
    fwrite(&synapse_count, sizeof(size_t), 1, file);

    fwrite(neurons, sizeof(Neuron_t), neuron_count, file);
    fwrite(synapses, sizeof(Synapse_t), synapse_count, file);

    fclose(file);
    printf("Neural network state saved to '%s'.\n", filename);
}
