#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "libttak/mem/arena.h"
#include "libttak/sched.h"
#include "neuron.h"
#include "../../common/motor/ioctl_car_cmd.h"

#define DEVNAME "/dev/motor"
#define SR04 "/dev/sr04"
#define NN_SAVE_FILE "neural_net.dat"

#define MOTOR_SPEED_MAX 100
#define SENSOR_DIST_MAX_CM 500
#define HOST_SIGNAL_MAX 100
#define RPE_LEARNING_RATE 0.1f

#define CONTROL_INTERVAL_US 100000
#define ARENA_HEAP_SIZE (sizeof(Neuron_t) * MAX_NEURONS + sizeof(Synapse_t) * MAX_SYNAPSES + sizeof(ttak_task_t) * 4 + 32768)

#define ATP_LEVEL_MAX 100.0f
#define ATP_REST_THRESHOLD 20.0f
#define ATP_BASE_CONSUMPTION 0.2f
#define ATP_MOTOR_CONSUMPTION 1.5f
#define ATP_RECOVERY_RATE 1.2f
#define SOCIAL_VOCAL_ATP_GATE 30.0f

#define ULTRA_PACKET_BITS 7
#define ULTRA_START_BIT (1u << 6)
#define ULTRA_STOP_BIT 0x01u
#define ULTRA_PULSE_SHORT_US 1200
#define ULTRA_PULSE_LONG_US 3200
#define ULTRA_INTERVAL_ZERO_NS 100000000L
#define ULTRA_INTERVAL_ONE_NS 200000000L
#define ULTRA_VOCALIZE_COOLDOWN_NS 1500000000L

typedef struct WormRuntime {
    float sensory_input[3];
    float motor_output[2];
    float prev_dist_input;
    float prev_host_input_l;
    float prev_host_input_r;
} WormRuntime_t;

typedef struct {
    int left_speed;
    int right_speed;
} MotorTelemetry_t;

static ttak_arena_t neural_arena;
static uint8_t neural_heap[ARENA_HEAP_SIZE];
static WormRuntime_t* worm_runtime = NULL;
static ttak_scheduler_t scheduler;
static ttak_task_t* scheduler_slots = NULL;

static int motor = -1;
static int sr04_sensor = -1;
static int sr04_comm = -1;
static bool sr04_comm_is_alias = false;

static float expected_reward = 0.0f;
static float exploration_drive = 0.0f;
static struct timespec last_vocalization_ts = {0};
static bool last_vocalization_valid = false;

static void read_sensors(float* sensory_input);
static float get_exploration_noise(void);
static MotorTelemetry_t send_motor_outputs(const float* motor_output, bool rest_mode);
static void worm_task(void* ctx);
static void update_energy_budget(float left_speed, float right_speed, bool rest_mode);
static void worm_vocalize(void);
static void worm_listen(void);
static void send_ultrasonic_packet(uint8_t packet);
static uint8_t receive_ultrasonic_packet(void);
static uint8_t determine_emotion_code(void);
static uint8_t determine_intensity_bits(uint8_t emotion_code);
static void apply_received_emotion(uint8_t emotion_code, uint8_t intensity_bits);
static struct timespec monotonic_now(void);
static int64_t timespec_diff_ns(const struct timespec* now, const struct timespec* past);
static void nanosleep_ns(long nanoseconds);

void sigHandler(int dummy) {
    (void)dummy;
    puts("Exiting...");
    NeuralNet_save(NN_SAVE_FILE);
    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    if (sr04_sensor >= 0) {
        close(sr04_sensor);
        sr04_sensor = -1;
    }
    if (!sr04_comm_is_alias && sr04_comm >= 0) {
        close(sr04_comm);
    }
    if (motor >= 0) {
        close(motor);
        motor = -1;
    }
    exit(0);
}

void read_sensors(float* sensory_input) {
    static float prev_raw_dist = 0.0f;
    char dist_buf[8] = {0};
    u_int32_t dist = 0;

    if (read(sr04_sensor, dist_buf, sizeof(dist_buf)) > 0) {
        dist = (u_int32_t)atoi(dist_buf);
    }

    float normalized_dist = 0.0f;
    if (dist > 0 && dist < SENSOR_DIST_MAX_CM) {
        normalized_dist = 1.0f - ((float)dist / SENSOR_DIST_MAX_CM);
    }

    float dist_delta = normalized_dist - prev_raw_dist;
    if (fabsf(dist_delta) < 0.01f) {
        float decayed = sensory_input[SENSOR_NEURON_DIST_IDX] * 0.9f;
        if (decayed <= 0.0005f) {
            sensory_input[SENSOR_NEURON_DIST_IDX] = 0.0f;
            if (exploration_drive < 0.5f) {
                norepinephrine_level = fminf(1.0f, norepinephrine_level + 0.3f);
                exploration_drive = 1.0f;
            }
        } else {
            sensory_input[SENSOR_NEURON_DIST_IDX] = decayed;
        }
    } else {
        sensory_input[SENSOR_NEURON_DIST_IDX] = normalized_dist;
    }
    prev_raw_dist = normalized_dist;

    if (normalized_dist > 0.5f) {
        float left_host_signal = 1.0f - (normalized_dist * 0.5f);
        float right_host_signal = 1.0f - (normalized_dist * 0.5f);
        if (rand() % 2 == 0) {
            sensory_input[SENSOR_NEURON_HOST_L_IDX] = left_host_signal;
            sensory_input[SENSOR_NEURON_HOST_R_IDX] = left_host_signal * 0.8f;
        } else {
            sensory_input[SENSOR_NEURON_HOST_L_IDX] = right_host_signal * 0.8f;
            sensory_input[SENSOR_NEURON_HOST_R_IDX] = right_host_signal;
        }
    } else {
        sensory_input[SENSOR_NEURON_HOST_L_IDX] = 1.0f;
        sensory_input[SENSOR_NEURON_HOST_R_IDX] = 1.0f;
    }
}

float get_exploration_noise(void) {
    float serotonin_noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * (serotonin_level * 0.5f);
    float norepi_component = 0.0f;
    if (exploration_drive > 0.0f) {
        norepi_component = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) *
                           (norepinephrine_level * exploration_drive);
        exploration_drive = fmaxf(0.0f, exploration_drive - 0.05f);
    }
    return serotonin_noise + norepi_component;
}

MotorTelemetry_t send_motor_outputs(const float* motor_output, bool rest_mode) {
    MotorTelemetry_t telemetry = {0, 0};
    struct ioctl_info io = {0};
    io.size = 5;

    int left_speed = (int)((motor_output[0] + get_exploration_noise()) * MOTOR_SPEED_MAX);
    int right_speed = (int)((motor_output[1] + get_exploration_noise()) * MOTOR_SPEED_MAX);

    if (!rest_mode && abs(left_speed) < 10 && abs(right_speed) < 10) {
        left_speed = 30;
        right_speed = 30;
    }

    if (rest_mode) {
        if (abs(left_speed) < 5) left_speed = 0;
        if (abs(right_speed) < 5) right_speed = 0;
    }

    if (left_speed < -MOTOR_SPEED_MAX) left_speed = -MOTOR_SPEED_MAX;
    if (right_speed < -MOTOR_SPEED_MAX) right_speed = -MOTOR_SPEED_MAX;
    if (left_speed > MOTOR_SPEED_MAX) left_speed = MOTOR_SPEED_MAX;
    if (right_speed > MOTOR_SPEED_MAX) right_speed = MOTOR_SPEED_MAX;

    io.buf[0] = 1;
    io.buf[1] = (left_speed >= 0);
    io.buf[2] = abs(left_speed);
    io.buf[3] = (right_speed >= 0);
    io.buf[4] = abs(right_speed);

    ioctl(motor, PI_CMD_IO, &io);

    telemetry.left_speed = left_speed;
    telemetry.right_speed = right_speed;

    printf("L: %d, R: %d | ATP: %.1f | Rest: %s | Dopamine: %.2f | Serotonin: %.2f | Norepi: %.2f | Cortisol: %.2f | Stability: %.2f\n",
           left_speed, right_speed, atp_level, rest_mode ? "YES" : "NO",
           dopamine_level, serotonin_level, norepinephrine_level, cortisol_level, stability_level);

    return telemetry;
}

void update_energy_budget(float left_speed, float right_speed, bool rest_mode) {
    float movement = (fabsf(left_speed) + fabsf(right_speed)) / (2.0f * MOTOR_SPEED_MAX);
    if (movement > 0.05f && !rest_mode) {
        float consumption = ATP_BASE_CONSUMPTION + movement * ATP_MOTOR_CONSUMPTION;
        atp_level = fmaxf(0.0f, atp_level - consumption);
    } else {
        float recovery = rest_mode ? ATP_RECOVERY_RATE * 1.5f : ATP_RECOVERY_RATE;
        atp_level = fminf(ATP_LEVEL_MAX, atp_level + recovery);
    }
}

static struct timespec monotonic_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

static int64_t timespec_diff_ns(const struct timespec* now, const struct timespec* past) {
    int64_t sec = (int64_t)now->tv_sec - (int64_t)past->tv_sec;
    int64_t nsec = (int64_t)now->tv_nsec - (int64_t)past->tv_nsec;
    return sec * 1000000000LL + nsec;
}

static void nanosleep_ns(long nanoseconds) {
    struct timespec ts = {
        .tv_sec = nanoseconds / 1000000000L,
        .tv_nsec = nanoseconds % 1000000000L
    };
    nanosleep(&ts, NULL);
}

static void sr04_toggle_trigger(bool high) {
    if (sr04_comm < 0) {
        return;
    }
    const char value = high ? '1' : '0';
    if (write(sr04_comm, &value, 1) < 0) {
        // Ignore write failures to keep control loop running
    }
}

static void sr04_emit_pulse(bool long_pulse) {
    sr04_toggle_trigger(true);
    nanosleep_ns((long)(long_pulse ? ULTRA_PULSE_LONG_US : ULTRA_PULSE_SHORT_US) * 1000L);
    sr04_toggle_trigger(false);
}

static void send_ultrasonic_bit(bool bit) {
    if (!bit) {
        sr04_emit_pulse(false);
        nanosleep_ns(ULTRA_INTERVAL_ZERO_NS);
        sr04_emit_pulse(false);
        nanosleep_ns(ULTRA_INTERVAL_ZERO_NS);
    } else {
        sr04_emit_pulse(true);
        nanosleep_ns(ULTRA_INTERVAL_ONE_NS);
    }
}

static uint8_t compose_emotion_packet(uint8_t emotion_code, uint8_t intensity_bits) {
    uint8_t packet = 0;
    packet |= ULTRA_START_BIT;
    packet |= (uint8_t)((emotion_code & 0x7u) << 3);
    packet |= (uint8_t)((intensity_bits & 0x3u) << 1);
    packet |= ULTRA_STOP_BIT;
    return packet & 0x7Fu;
}

static uint8_t determine_emotion_code(void) {
    if (epinephrine_level > 0.85f && cortisol_level > 0.8f) {
        return 0x06; // Terror
    }
    if (cortisol_level > 0.8f) {
        return 0x03; // Pain
    }
    if (dopamine_level > 0.5f && oxytocin_level > 0.5f) {
        return 0x02; // Bond
    }
    if (gaba_level > 0.65f && stability_level < 0.3f) {
        return 0x05; // Repel
    }
    if (dopamine_level > 0.4f && acetylcholine_level > 0.5f) {
        return 0x04; // Attract
    }
    if (atp_level < 10.0f || serotonin_level < 0.2f) {
        return 0x01; // Empty
    }
    if (stability_level > 0.6f && serotonin_level > 0.4f) {
        return 0x00; // Fine
    }
    if (norepinephrine_level > 0.7f) {
        return 0x06; // Terror fallback
    }
    return 0x00;
}

static uint8_t determine_intensity_bits(uint8_t emotion_code) {
    float measure = 0.0f;
    switch (emotion_code) {
        case 0x00: measure = stability_level; break;
        case 0x01: measure = 1.0f - fminf(1.0f, serotonin_level); break;
        case 0x02: measure = (dopamine_level + oxytocin_level) * 0.5f; break;
        case 0x03: measure = fmaxf(cortisol_level, epinephrine_level); break;
        case 0x04: measure = fmaxf(dopamine_level, acetylcholine_level); break;
        case 0x05: measure = fmaxf(gaba_level, 1.0f - stability_level); break;
        case 0x06: measure = fmaxf(norepinephrine_level, epinephrine_level); break;
        default: measure = 0.0f; break;
    }
    if (measure < 0.0f) measure = 0.0f;
    if (measure > 1.0f) measure = 1.0f;
    int level = (int)lrintf(measure * 3.0f);
    if (level < 0) level = 0;
    if (level > 3) level = 3;
    return (uint8_t)level;
}

static void send_ultrasonic_packet(uint8_t packet) {
    if (sr04_comm < 0) {
        return;
    }
    for (int bit = ULTRA_PACKET_BITS - 1; bit >= 0; --bit) {
        bool bit_value = ((packet >> bit) & 0x1u) != 0;
        send_ultrasonic_bit(bit_value);
    }
}

static void worm_vocalize(void) {
    if (atp_level > SOCIAL_VOCAL_ATP_GATE || sr04_comm < 0) {
        return;
    }
    struct timespec now = monotonic_now();
    if (last_vocalization_valid) {
        int64_t delta = timespec_diff_ns(&now, &last_vocalization_ts);
        if (delta < ULTRA_VOCALIZE_COOLDOWN_NS) {
            return;
        }
    }
    uint8_t emotion_code = determine_emotion_code();
    uint8_t intensity_bits = determine_intensity_bits(emotion_code);
    uint8_t packet = compose_emotion_packet(emotion_code, intensity_bits);
    send_ultrasonic_packet(packet);
    last_vocalization_ts = now;
    last_vocalization_valid = true;
}

static float intensity_scalar(uint8_t intensity_bits) {
    switch (intensity_bits & 0x3u) {
        case 0: return 0.25f;
        case 1: return 0.5f;
        case 2: return 0.75f;
        case 3: return 1.0f;
    }
    return 0.25f;
}

static uint8_t receive_ultrasonic_packet(void) {
    if (sr04_comm < 0 || sr04_comm_is_alias) {
        return 0xFFu;
    }
    uint8_t raw = 0;
    ssize_t bytes = read(sr04_comm, &raw, sizeof(raw));
    if (bytes <= 0) {
        if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 0xFFu;
        }
        return 0xFFu;
    }
    if ((raw & ULTRA_START_BIT) == 0 || (raw & ULTRA_STOP_BIT) == 0) {
        return 0xFFu;
    }
    return raw & 0x7Fu;
}

static void apply_received_emotion(uint8_t emotion_code, uint8_t intensity_bits) {
    float scalar = intensity_scalar(intensity_bits);
    switch (emotion_code & 0x7u) {
        case 0x00: // Fine
            serotonin_level = fminf(1.0f, serotonin_level + 0.05f * scalar);
            stability_level = fminf(1.0f, stability_level + 0.03f * scalar);
            break;
        case 0x01: // Empty
            exploration_drive = fminf(1.5f, exploration_drive + 0.2f * scalar);
            norepinephrine_level = fminf(1.0f, norepinephrine_level + 0.05f * scalar);
            break;
        case 0x02: // Bond
            oxytocin_level = fminf(1.0f, oxytocin_level + 0.1f * scalar);
            stability_level = fminf(1.0f, stability_level + 0.08f * scalar);
            break;
        case 0x03: // Pain
            cortisol_level = fminf(1.0f, cortisol_level + 0.15f * scalar);
            epinephrine_level = fminf(1.0f, epinephrine_level + 0.2f * scalar);
            break;
        case 0x04: // Attract
            dopamine_level = fminf(1.0f, dopamine_level + 0.08f * scalar);
            acetylcholine_level = fminf(1.0f, acetylcholine_level + 0.05f * scalar);
            break;
        case 0x05: // Repel
            dopamine_level = fmaxf(-1.0f, dopamine_level - 0.05f * scalar);
            stability_level = fmaxf(0.0f, stability_level - 0.07f * scalar);
            gaba_level = fminf(1.0f, gaba_level + 0.04f * scalar);
            break;
        case 0x06: // Terror
            epinephrine_level = fminf(1.0f, epinephrine_level + 0.25f * scalar);
            cortisol_level = fminf(1.0f, cortisol_level + 0.2f * scalar);
            stability_level = fmaxf(0.0f, stability_level - 0.15f * scalar);
            break;
        default:
            break;
    }
}

static void worm_listen(void) {
    for (int i = 0; i < 4; ++i) {
        uint8_t packet = receive_ultrasonic_packet();
        if (packet == 0xFFu) {
            break;
        }
        uint8_t emotion = (packet >> 3) & 0x7u;
        uint8_t intensity = (packet >> 1) & 0x3u;
        apply_received_emotion(emotion, intensity);
    }
}

void worm_task(void* ctx) {
    WormRuntime_t* runtime = (WormRuntime_t*)ctx;

    runtime->prev_dist_input = runtime->sensory_input[SENSOR_NEURON_DIST_IDX];
    runtime->prev_host_input_l = runtime->sensory_input[SENSOR_NEURON_HOST_L_IDX];
    runtime->prev_host_input_r = runtime->sensory_input[SENSOR_NEURON_HOST_R_IDX];

    read_sensors(runtime->sensory_input);

    float max_sensory_input = fmaxf(runtime->sensory_input[SENSOR_NEURON_DIST_IDX],
                                    fmaxf(runtime->sensory_input[SENSOR_NEURON_HOST_L_IDX],
                                          runtime->sensory_input[SENSOR_NEURON_HOST_R_IDX]));
    acetylcholine_level = fminf(1.0f, max_sensory_input * 1.5f);

    float conflicting_signals = fabsf(runtime->sensory_input[SENSOR_NEURON_DIST_IDX] -
                                      (runtime->sensory_input[SENSOR_NEURON_HOST_L_IDX] +
                                       runtime->sensory_input[SENSOR_NEURON_HOST_R_IDX]) / 2.0f);
    gaba_level = fminf(1.0f, conflicting_signals);

    if (runtime->sensory_input[SENSOR_NEURON_DIST_IDX] > 0.9f) {
        epinephrine_level = 1.0f;
        cortisol_level = fminf(1.0f, cortisol_level + 0.2f);
    } else {
        epinephrine_level *= 0.99f;
        cortisol_level *= 0.995f;
    }

    if (max_sensory_input > 0.5f) {
        norepinephrine_level = fminf(1.0f, norepinephrine_level + 0.1f);
        glutamate_level = fminf(1.0f, glutamate_level + 0.1f);
    } else {
        norepinephrine_level *= 0.99f;
        glutamate_level *= 0.99f;
    }

    bool getting_closer_to_obstacle = (runtime->sensory_input[SENSOR_NEURON_DIST_IDX] >
                                       runtime->prev_dist_input + 0.01f);
    bool getting_closer_to_host = (runtime->sensory_input[SENSOR_NEURON_HOST_L_IDX] >
                                   runtime->prev_host_input_l + 0.01f &&
                                   runtime->sensory_input[SENSOR_NEURON_HOST_R_IDX] >
                                   runtime->prev_host_input_r + 0.01f);
    bool getting_further_from_host = (runtime->sensory_input[SENSOR_NEURON_HOST_L_IDX] <
                                      runtime->prev_host_input_l - 0.01f &&
                                      runtime->sensory_input[SENSOR_NEURON_HOST_R_IDX] <
                                      runtime->prev_host_input_r - 0.01f);

    float actual_reward = 0.0f;
    if (getting_closer_to_host) {
        actual_reward = 1.0f;
        endorphin_level = fminf(1.0f, endorphin_level + 0.1f);
        oxytocin_level = fminf(1.0f, oxytocin_level + 0.1f);
    } else if (getting_further_from_host) {
        actual_reward = -0.5f;
        endorphin_level *= 0.99f;
        oxytocin_level *= 0.99f;
    } else if (getting_closer_to_obstacle) {
        actual_reward = -1.0f;
        endorphin_level *= 0.99f;
        oxytocin_level *= 0.99f;
    } else {
        endorphin_level *= 0.995f;
        oxytocin_level *= 0.995f;
    }

    dopamine_level = actual_reward - expected_reward;
    expected_reward = expected_reward + RPE_LEARNING_RATE * dopamine_level;

    if (dopamine_level > 0.0f) {
        serotonin_level *= 0.99f;
    } else {
        serotonin_level = fminf(1.0f, serotonin_level + 0.01f);
    }

    float hormonal_flux = fabsf(dopamine_level) + fabsf(serotonin_level);
    if (hormonal_flux < 0.2f && actual_reward >= 0.0f) {
        stability_level = fminf(1.0f, stability_level + 0.05f);
    } else {
        stability_level = fmaxf(0.0f, stability_level - 0.1f);
    }

    bool rest_mode = (atp_level <= ATP_REST_THRESHOLD);
    if (rest_mode) {
        serotonin_level = fminf(1.0f, fmaxf(serotonin_level, 0.8f));
    }

    worm_listen();
    worm_vocalize();

    NeuralNet_step(runtime->sensory_input, runtime->motor_output);

    float final_left_output = runtime->motor_output[0];
    float final_right_output = runtime->motor_output[1];

    if (epinephrine_level > 0.5f) {
        final_left_output = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
        final_right_output = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
    }

    if (rest_mode) {
        final_left_output *= 0.2f;
        final_right_output *= 0.2f;
    }

    float final_motor_output[2] = {final_left_output, final_right_output};
    MotorTelemetry_t telemetry = send_motor_outputs(final_motor_output, rest_mode);

    update_energy_budget((float)telemetry.left_speed, (float)telemetry.right_speed, rest_mode);
}

int main(void) {
    motor = open(DEVNAME, O_RDWR);
    if (motor < 0) { perror("Failed to open motor device"); return -1; }
    sr04_sensor = open(SR04, O_RDWR);
    if (sr04_sensor < 0) { perror("Failed to open sr04 device"); close(motor); return -1; }
    sr04_comm = open(SR04, O_RDWR | O_NONBLOCK);
    if (sr04_comm < 0) {
        perror("SR04 social channel unavailable, falling back to sensor descriptor");
        sr04_comm = sr04_sensor;
        sr04_comm_is_alias = true;
    }

    signal(SIGINT, sigHandler);
    srand((unsigned int)time(NULL));

    ttak_arena_init(&neural_arena, neural_heap, sizeof(neural_heap));
    worm_runtime = (WormRuntime_t*)ttak_arena_alloc(&neural_arena, sizeof(WormRuntime_t), sizeof(void*));
    memset(worm_runtime, 0, sizeof(WormRuntime_t));

    scheduler_slots = (ttak_task_t*)ttak_arena_alloc(&neural_arena, sizeof(ttak_task_t) * 4, sizeof(void*));
    ttak_sched_init(&scheduler, scheduler_slots, 4);

    NeuralNet_load(NN_SAVE_FILE, &neural_arena);

    ttak_sched_add(&scheduler, worm_task, worm_runtime, CONTROL_INTERVAL_US);

    puts("Neural network control loop started. Learning enabled.");
    ttak_sched_run_loop(&scheduler);

    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    close(motor);
    if (!sr04_comm_is_alias && sr04_comm >= 0) {
        close(sr04_comm);
    }
    if (sr04_sensor >= 0) {
        close(sr04_sensor);
    }
    return 0;
}
