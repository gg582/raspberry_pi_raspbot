#include "neuron.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAVE_FILE_VERSION 1U
#define FX_WEIGHT_MIN TTAK_FX_CONST(-1.5f)
#define FX_WEIGHT_MAX TTAK_FX_CONST(1.5f)
#define FX_STRENGTH_RECOVERY TTAK_FX_CONST(0.001f)
#define FX_STRENGTH_FATIGUE TTAK_FX_CONST(0.99f)
#define ELIGIBILITY_DECAY_FACTOR TTAK_FX_CONST(0.95f)
#define ELIGIBILITY_FLOOR TTAK_FX_CONST(0.05f)
#define LEARNING_RATE_FX TTAK_FX_CONST(0.02f)
#define ATP_STEP_DRAIN 0.02f

typedef struct {
    uint32_t version;
    uint32_t delta_count;
} NeuralNetSaveHeader;

typedef struct {
    uint32_t index;
    ttak_fx_t weight_delta;
    ttak_fx_t strength_delta;
} SynapseDelta;

typedef struct {
    NeuronType_t type;
    const char* token;
} TypeRule;

static Neuron_t* neurons = NULL;
static Synapse_t* synapses = NULL;
static ttak_arena_t* neural_arena = NULL;

size_t neuron_count = 0;
size_t synapse_count = 0;

float dopamine_level = 0.0f;
float serotonin_level = 0.0f;
float acetylcholine_level = 0.0f;
float gaba_level = 0.0f;
float epinephrine_level = 0.0f;
float norepinephrine_level = 0.0f;
float glutamate_level = 0.0f;
float endorphin_level = 0.0f;
float oxytocin_level = 0.0f;
float cortisol_level = 0.0f;
float stability_level = 0.0f;
float atp_level = 100.0f;

static void ensure_buffers(ttak_arena_t* arena) {
    if (arena == NULL) {
        return;
    }

    if (neural_arena == NULL) {
        neural_arena = arena;
    }

    if (neurons == NULL) {
        neurons = (Neuron_t*)ttak_arena_alloc(neural_arena, sizeof(Neuron_t) * MAX_NEURONS, sizeof(void*));
    }
    if (synapses == NULL) {
        synapses = (Synapse_t*)ttak_arena_alloc(neural_arena, sizeof(Synapse_t) * MAX_SYNAPSES, sizeof(void*));
    }
}

static void reset_neurons(void) {
    static const TypeRule rules[] = {
        {NEURON_TYPE_SENSORY, "SR04_DIST"},
        {NEURON_TYPE_SENSORY, "HOST_SIGNAL"},
        {NEURON_TYPE_MOTOR, "MOTOR"},
        {NEURON_TYPE_EXCITATORY, "_EX_"},
        {NEURON_TYPE_INHIBITORY, "_IN_"},
    };

    for (size_t i = 0; i < neuron_count; ++i) {
        strncpy(neurons[i].name, neuron_names[i], sizeof(neurons[i].name) - 1);
        neurons[i].name[sizeof(neurons[i].name) - 1] = '\0';
        neurons[i].activation_fx = 0;
        neurons[i].previous_activation_fx = 0;
        neurons[i].type = NEURON_TYPE_EXCITATORY;
        for (size_t r = 0; r < sizeof(rules) / sizeof(TypeRule); ++r) {
            if (strstr(neurons[i].name, rules[r].token) != NULL) {
                neurons[i].type = rules[r].type;
                break;
            }
        }
    }
}

static void bootstrap_network(ttak_arena_t* arena) {
    ensure_buffers(arena);
    neuron_count = NUM_NEURONS_INIT;
    init_synapses_biological();
    synapse_count = NUM_SYNAPSES_INIT;

    for (size_t i = 0; i < synapse_count; ++i) {
        synapses[i] = neural_synapses[i];
        synapses[i].eligibility_trace_fx = 0;
    }

    reset_neurons();
}

static ttak_fx_t get_neuron_threshold_fx(NeuronType_t type) {
    (void)type;
    ttak_fx_t threshold = TTAK_FX_CONST(0.1f);
    threshold = ttak_fx_add(threshold, ttak_fx_from_float(serotonin_level * 0.1f));
    threshold = ttak_fx_add(threshold, ttak_fx_from_float(gaba_level * 0.3f));
    threshold = ttak_fx_add(threshold, ttak_fx_from_float(endorphin_level * 0.1f));
    threshold = ttak_fx_add(threshold, ttak_fx_from_float(cortisol_level * 0.2f));

    threshold = ttak_fx_sub(threshold, ttak_fx_from_float(dopamine_level * 0.1f));
    threshold = ttak_fx_sub(threshold, ttak_fx_from_float(acetylcholine_level * 0.2f));
    threshold = ttak_fx_sub(threshold, ttak_fx_from_float(norepinephrine_level * 0.3f));
    threshold = ttak_fx_sub(threshold, ttak_fx_from_float(glutamate_level * 0.1f));
    threshold = ttak_fx_sub(threshold, ttak_fx_from_float(stability_level * 0.1f));

    return threshold;
}

static void apply_temporal_credit(void) {
    float dopamine = dopamine_level;
    if (fabsf(dopamine) < 0.0005f) {
        return;
    }

    ttak_fx_t dopamine_fx = ttak_fx_from_float(fabsf(dopamine));
    ttak_fx_t glutamate_fx = ttak_fx_from_float(fmaxf(0.1f, glutamate_level));

    for (size_t i = 0; i < synapse_count; ++i) {
        if (synapses[i].eligibility_trace_fx < ELIGIBILITY_FLOOR) {
            continue;
        }

        ttak_fx_t plasticity = ttak_fx_mul(LEARNING_RATE_FX, dopamine_fx);
        plasticity = ttak_fx_mul(plasticity, glutamate_fx);
        plasticity = ttak_fx_mul(plasticity, synapses[i].eligibility_trace_fx);

        if (dopamine < 0.0f) {
            plasticity = -plasticity;
        }

        synapses[i].weight_fx = ttak_fx_clamp(
            ttak_fx_add(synapses[i].weight_fx, plasticity),
            FX_WEIGHT_MIN,
            FX_WEIGHT_MAX);

        synapses[i].eligibility_trace_fx = ttak_fx_decay(synapses[i].eligibility_trace_fx, TTAK_FX_CONST(0.5f));
    }
}

void NeuralNet_init(ttak_arena_t* arena) {
    bootstrap_network(arena);
}

void NeuralNet_step(const float* sensory_input, float* motor_output) {
    if (!neurons || !synapses) {
        return;
    }

    atp_level = fmaxf(0.0f, atp_level - ATP_STEP_DRAIN);

    for (size_t i = 0; i < neuron_count; ++i) {
        neurons[i].previous_activation_fx = neurons[i].activation_fx;
    }

    for (size_t i = 3; i < neuron_count; ++i) {
        neurons[i].activation_fx = 0;
    }

    for (size_t i = 0; i < 3; ++i) {
        float bounded = sensory_input ? sensory_input[i] : 0.0f;
        if (bounded > 1.0f) bounded = 1.0f;
        if (bounded < -1.0f) bounded = -1.0f;
        neurons[i].activation_fx = ttak_fx_swish(ttak_fx_from_float(bounded));
    }

    for (size_t i = 0; i < synapse_count; ++i) {
        int from = synapses[i].from;
        int to = synapses[i].to;

        ttak_fx_t threshold_from = get_neuron_threshold_fx(neurons[from].type);
        ttak_fx_t threshold_to = get_neuron_threshold_fx(neurons[to].type);

        ttak_fx_t mixed_input = ttak_fx_add(
            ttak_fx_mul(neurons[from].activation_fx, TTAK_FX_CONST(0.7f)),
            ttak_fx_mul(neurons[from].previous_activation_fx, TTAK_FX_CONST(0.3f)));

        ttak_fx_t weighted = ttak_fx_mul(ttak_fx_mul(mixed_input, synapses[i].weight_fx),
                                         synapses[i].synaptic_strength_fx);

        if (weighted > threshold_to) {
            neurons[to].activation_fx = ttak_fx_add(neurons[to].activation_fx, weighted);
        }

        if (neurons[from].activation_fx > threshold_from) {
            synapses[i].synaptic_strength_fx = ttak_fx_mul(synapses[i].synaptic_strength_fx, FX_STRENGTH_FATIGUE);
            synapses[i].eligibility_trace_fx = TTAK_FX_ONE;
        } else {
            synapses[i].eligibility_trace_fx = ttak_fx_decay(synapses[i].eligibility_trace_fx, ELIGIBILITY_DECAY_FACTOR);
        }

        if (synapses[i].synaptic_strength_fx < TTAK_FX_ONE) {
            synapses[i].synaptic_strength_fx = ttak_fx_add(synapses[i].synaptic_strength_fx, FX_STRENGTH_RECOVERY);
            if (synapses[i].synaptic_strength_fx > TTAK_FX_ONE) {
                synapses[i].synaptic_strength_fx = TTAK_FX_ONE;
            }
        }
    }

    for (size_t i = 0; i < neuron_count; ++i) {
        if (neurons[i].type == NEURON_TYPE_EXCITATORY || neurons[i].type == NEURON_TYPE_INHIBITORY) {
            neurons[i].activation_fx = ttak_fx_swish(neurons[i].activation_fx);
        }
    }

    if (serotonin_level > 0.5f) {
        for (size_t i = 0; i < synapse_count; ++i) {
            if (synapses[i].neurotransmitter_type_fx < 0) {
                synapses[i].weight_fx = ttak_fx_mul(synapses[i].weight_fx, TTAK_FX_CONST(0.99f));
            }
        }
    }

    apply_temporal_credit();

    if (motor_output) {
        motor_output[0] = ttak_fx_to_float(ttak_fx_swish(neurons[MOTOR_NEURON_L_IDX].activation_fx));
        motor_output[1] = ttak_fx_to_float(ttak_fx_swish(neurons[MOTOR_NEURON_R_IDX].activation_fx));
    }
}

void NeuralNet_load(const char* filename, ttak_arena_t* arena) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("No saved neural network found. Initializing a new one.\n");
        NeuralNet_init(arena);
        return;
    }

    NeuralNetSaveHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1 || header.version != SAVE_FILE_VERSION) {
        printf("Saved state mismatch. Initializing a new neural network.\n");
        fclose(file);
        NeuralNet_init(arena);
        return;
    }

    bootstrap_network(arena);

    for (uint32_t i = 0; i < header.delta_count; ++i) {
        SynapseDelta delta;
        if (fread(&delta, sizeof(delta), 1, file) != 1) {
            break;
        }
        if (delta.index >= synapse_count) {
            continue;
        }

        synapses[delta.index].weight_fx = ttak_fx_add(neural_synapses[delta.index].weight_fx, delta.weight_delta);
        synapses[delta.index].synaptic_strength_fx = ttak_fx_add(neural_synapses[delta.index].synaptic_strength_fx,
                                                                 delta.strength_delta);
    }

    fclose(file);
    printf("Neural network state loaded successfully from '%s'. Applied %u offsets.\n",
           filename, header.delta_count);
}

void NeuralNet_save(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Failed to save neural network");
        return;
    }

    SynapseDelta deltas[MAX_SYNAPSES];
    size_t delta_count = 0;
    for (size_t i = 0; i < synapse_count; ++i) {
        ttak_fx_t weight_delta = ttak_fx_sub(synapses[i].weight_fx, neural_synapses[i].weight_fx);
        ttak_fx_t strength_delta = ttak_fx_sub(synapses[i].synaptic_strength_fx, neural_synapses[i].synaptic_strength_fx);

        if (weight_delta != 0 || strength_delta != 0) {
            deltas[delta_count].index = (uint32_t)i;
            deltas[delta_count].weight_delta = weight_delta;
            deltas[delta_count].strength_delta = strength_delta;
            ++delta_count;
        }
    }

    NeuralNetSaveHeader header = {
        .version = SAVE_FILE_VERSION,
        .delta_count = (uint32_t)delta_count,
    };

    fwrite(&header, sizeof(header), 1, file);
    fwrite(deltas, sizeof(SynapseDelta), delta_count, file);
    fclose(file);

    printf("Neural network state saved to '%s' with %zu offsets.\n", filename, delta_count);
}
