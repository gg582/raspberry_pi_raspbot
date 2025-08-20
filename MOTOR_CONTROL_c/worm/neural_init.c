#include "neuron.h"
#include <stdlib.h>
#include <string.h>

/*
 * Definition of neuron names used in the network.
 */
const char* neuron_names[MAX_NEURONS] = {
    // Sensory Neurons (3)
    "SR04_DIST",          // Distance sensor (avoids obstacles)
    "HOST_SIGNAL_L",      // Host signal sensor (finds host, left side)
    "HOST_SIGNAL_R",      // Host signal sensor (finds host, right side)

    // Inter Neurons (75)
    // Group 1: Host Signal Excitatory (20)
    "INTER_H_EX_0", "INTER_H_EX_1", "INTER_H_EX_2", "INTER_H_EX_3", "INTER_H_EX_4",
    "INTER_H_EX_5", "INTER_H_EX_6", "INTER_H_EX_7", "INTER_H_EX_8", "INTER_H_EX_9",
    "INTER_H_EX_10", "INTER_H_EX_11", "INTER_H_EX_12", "INTER_H_EX_13", "INTER_H_EX_14",
    "INTER_H_EX_15", "INTER_H_EX_16", "INTER_H_EX_17", "INTER_H_EX_18", "INTER_H_EX_19",

    // Group 2: Host Signal Inhibitory (18)
    "INTER_H_IN_0", "INTER_H_IN_1", "INTER_H_IN_2", "INTER_H_IN_3", "INTER_H_IN_4",
    "INTER_H_IN_5", "INTER_H_IN_6", "INTER_H_IN_7", "INTER_H_IN_8", "INTER_H_IN_9",
    "INTER_H_IN_10", "INTER_H_IN_11", "INTER_H_IN_12", "INTER_H_IN_13", "INTER_H_IN_14",
    "INTER_H_IN_15", "INTER_H_IN_16", "INTER_H_IN_17",

    // Group 3: Avoidance Excitatory (20)
    "INTER_A_EX_0", "INTER_A_EX_1", "INTER_A_EX_2", "INTER_A_EX_3", "INTER_A_EX_4",
    "INTER_A_EX_5", "INTER_A_EX_6", "INTER_A_EX_7", "INTER_A_EX_8", "INTER_A_EX_9",
    "INTER_A_EX_10", "INTER_A_EX_11", "INTER_A_EX_12", "INTER_A_EX_13", "INTER_A_EX_14",
    "INTER_A_EX_15", "INTER_A_EX_16", "INTER_A_EX_17", "INTER_A_EX_18", "INTER_A_EX_19",

    // Group 4: Avoidance Inhibitory (17)
    "INTER_A_IN_0", "INTER_A_IN_1", "INTER_A_IN_2", "INTER_A_IN_3", "INTER_A_IN_4",
    "INTER_A_IN_5", "INTER_A_IN_6", "INTER_A_IN_7", "INTER_A_IN_8", "INTER_A_IN_9",
    "INTER_A_IN_10", "INTER_A_IN_11", "INTER_A_IN_12", "INTER_A_IN_13", "INTER_A_IN_14",
    "INTER_A_IN_15", "INTER_A_IN_16",

    // Motor Neurons (2)
    "MOTOR_LEFT",
    "MOTOR_RIGHT"
};

Synapse_t neural_synapses[MAX_SYNAPSES];

const size_t NUM_NEURONS_INIT = 80;
size_t NUM_SYNAPSES_INIT = 0;

void init_synapses_biological(void) {
    // Sensory Neurons
    const int SENSORY_DIST = 0;
    const int SENSORY_HOST_L = 1;
    const int SENSORY_HOST_R = 2;

    // Inter Neuron Groups
    const int INTER_H_EX_START = 3;
    const int INTER_H_EX_END = 22;
    const int INTER_H_IN_START = 23;
    const int INTER_H_IN_END = 40;
    const int INTER_A_EX_START = 41;
    const int INTER_A_EX_END = 60;
    const int INTER_A_IN_START = 61;
    const int INTER_A_IN_END = 77;

    // Motor Neurons
    const int MOTOR_L = 78;
    const int MOTOR_R = 79;

    // 1. Host Signal -> Inter Neurons (Goal-Oriented)
    for (int i = INTER_H_EX_START; i <= INTER_H_EX_END; i++) {
        float random_weight = ((float)rand() / (float)RAND_MAX) - 0.5f;
        neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){SENSORY_HOST_L, i, random_weight, 1.0f, 1.0f, SYNAPSE_TYPE_NORMAL};
        neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){SENSORY_HOST_R, i, random_weight, 1.0f, 1.0f, SYNAPSE_TYPE_NORMAL};
    }

    // 2. Avoidance Signal -> Inter Neurons (Reactive)
    for (int i = INTER_A_EX_START; i <= INTER_A_EX_END; i++) {
        float random_weight = ((float)rand() / (float)RAND_MAX) - 0.5f;
        neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){SENSORY_DIST, i, random_weight, 1.0f, -1.0f, SYNAPSE_TYPE_NORMAL};
    }

    // 3. Inter -> Inter (Local Circuits)
    // Host Excitatory -> Host Inhibitory
    for (int i = INTER_H_EX_START; i <= INTER_H_EX_END; i++) {
        for (int j = INTER_H_IN_START; j <= INTER_H_IN_END; j++) {
            if (rand() % 5 == 0) { // 20% connection probability
                float random_weight = -(((float)rand() / (float)RAND_MAX) * 0.5f);
                neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){i, j, random_weight, 1.0f, -1.0f, SYNAPSE_TYPE_NORMAL};
            }
        }
    }
    // Avoidance Excitatory -> Avoidance Inhibitory
    for (int i = INTER_A_EX_START; i <= INTER_A_EX_END; i++) {
        for (int j = INTER_A_IN_START; j <= INTER_A_IN_END; j++) {
            if (rand() % 5 == 0) {
                float random_weight = -(((float)rand() / (float)RAND_MAX) * 0.5f);
                neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){i, j, random_weight, 1.0f, -1.0f, SYNAPSE_TYPE_NORMAL};
            }
        }
    }

    // 4. Inter -> Motor Neurons (Goal vs. Avoidance)
    // Host Interneurons -> Motor Neurons (Excitatory to move forward)
    for (int i = INTER_H_EX_START; i <= INTER_H_EX_END; i++) {
        float random_weight = ((float)rand() / (float)RAND_MAX) * 0.2f;
        neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){i, MOTOR_L, random_weight, 1.0f, 1.0f, SYNAPSE_TYPE_NORMAL};
        neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){i, MOTOR_R, random_weight, 1.0f, 1.0f, SYNAPSE_TYPE_NORMAL};
    }
    // Avoidance Interneurons -> Motor Neurons (Inhibitory to stop/turn)
    for (int i = INTER_A_EX_START; i <= INTER_A_EX_END; i++) {
        float random_weight = -(((float)rand() / (float)RAND_MAX) * 0.2f);
        neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){i, MOTOR_L, random_weight, 1.0f, -1.0f, SYNAPSE_TYPE_NORMAL};
        neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){i, MOTOR_R, random_weight, 1.0f, -1.0f, SYNAPSE_TYPE_NORMAL};
    }
    
    // 5. Antagonistic connections between motor neurons
    neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){MOTOR_L, MOTOR_R, -0.5f, 1.0f, -1.0f, SYNAPSE_TYPE_ANTAGONISTIC};
    neural_synapses[NUM_SYNAPSES_INIT++] = (Synapse_t){MOTOR_R, MOTOR_L, -0.5f, 1.0f, -1.0f, SYNAPSE_TYPE_ANTAGONISTIC};
}
