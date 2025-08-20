#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <math.h>

#include "neuron.h"
#include "../../common/motor/ioctl_car_cmd.h"

// Device file paths
#define DEVNAME "/dev/motor"
#define SR04 "/dev/sr04"
#define NN_SAVE_FILE "neural_net.dat"

// Indices for neurons
#define SENSOR_NEURON_DIST_IDX 0
#define SENSOR_NEURON_HOST_L_IDX 1
#define SENSOR_NEURON_HOST_R_IDX 2
#define MOTOR_NEURON_L_IDX 78
#define MOTOR_NEURON_R_IDX 79

// System parameters
#define MOTOR_SPEED_MAX 100
#define SENSOR_DIST_MAX_CM 500
#define HOST_SIGNAL_MAX 100
#define RPE_LEARNING_RATE 0.1f

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
extern float stability_level;

int motor;
int sr04_l;
int sr04_r;

float expected_reward = 0.0f;

void sigHandler(int dummy) {
    puts("Exiting...");
    NeuralNet_save(NN_SAVE_FILE);
    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    close(sr04_l);
    close(sr04_r);
    close(motor);
    exit(0);
}

// Corrected read_sensors function to tie host signal to obstacle avoidance
void read_sensors(float* sensory_input) {
    char dist_buf[8];
    u_int32_t dist;

    read(sr04_l, dist_buf, sizeof(dist_buf));
    dist = atoi(dist_buf);

    float normalized_dist = 0.0f;
    if (dist > 0 && dist < SENSOR_DIST_MAX_CM) {
        normalized_dist = 1.0f - ((float)dist / SENSOR_DIST_MAX_CM);
    }
    
    // Obstacle signal is simple
    sensory_input[SENSOR_NEURON_DIST_IDX] = normalized_dist;

    // NEW: Host signal is now a function of obstacle avoidance
    // We assume the host signal is strongest where there is no obstacle
    if (normalized_dist > 0.5f) {
        // Assume obstacle is close, so generate a host signal on the side
        // that's farther away from the simulated obstacle.
        float left_host_signal = 1.0f - (normalized_dist * 0.5f);
        float right_host_signal = 1.0f - (normalized_dist * 0.5f);

        // Here we can introduce a bias to simulate the obstacle being more on one side
        if (rand() % 2 == 0) {
            sensory_input[SENSOR_NEURON_HOST_L_IDX] = left_host_signal;
            sensory_input[SENSOR_NEURON_HOST_R_IDX] = left_host_signal * 0.8f;
        } else {
            sensory_input[SENSOR_NEURON_HOST_L_IDX] = right_host_signal * 0.8f;
            sensory_input[SENSOR_NEURON_HOST_R_IDX] = right_host_signal;
        }
    } else {
        // No obstacle, so generate a strong host signal directly ahead
        sensory_input[SENSOR_NEURON_HOST_L_IDX] = 1.0f;
        sensory_input[SENSOR_NEURON_HOST_R_IDX] = 1.0f;
    }
}

float get_exploration_noise() {
    return ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * serotonin_level;
}

void send_motor_outputs(float* motor_output) {
    struct ioctl_info io;
    io.size = 5;
    int left_speed, right_speed;

    left_speed = (int)((motor_output[0] + get_exploration_noise()) * MOTOR_SPEED_MAX);
    right_speed = (int)((motor_output[1] + get_exploration_noise()) * MOTOR_SPEED_MAX);

    if (fabs(left_speed) < 10 && fabs(right_speed) < 10) {
        left_speed = 30;
        right_speed = 30;
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

    printf("L: %d, R: %d | Dopamine(RPE): %.2f | Serotonin: %.2f | Epinephrine: %.2f | Cortisol: %.2f | Oxytocin: %.2f | GABA: %.2f | Stability: %.2f\n",
           left_speed, right_speed, dopamine_level, serotonin_level, epinephrine_level, cortisol_level, oxytocin_level, gaba_level, stability_level);
}

int main(void) {
    motor = open(DEVNAME, O_RDWR);
    if (motor < 0) { perror("Failed to open motor device"); return -1; }
    sr04_l = open(SR04, O_RDWR);
    if (sr04_l < 0) { perror("Failed to open sr04 device"); close(motor); return -1; }
    sr04_r = sr04_l;

    signal(SIGINT, sigHandler);
    srand(time(NULL));

    NeuralNet_load(NN_SAVE_FILE);

    float sensory_input[3] = {0};
    float motor_output[2] = {0};
    float prev_dist_input = 0.0f;
    float prev_host_input_l = 0.0f;
    float prev_host_input_r = 0.0f;

    puts("Neural network control loop started. Learning enabled.");

    while (true) {
        prev_dist_input = sensory_input[SENSOR_NEURON_DIST_IDX];
        prev_host_input_l = sensory_input[SENSOR_NEURON_HOST_L_IDX];
        prev_host_input_r = sensory_input[SENSOR_NEURON_HOST_R_IDX];

        read_sensors(sensory_input);

        float max_sensory_input = fmax(sensory_input[SENSOR_NEURON_DIST_IDX], fmax(sensory_input[SENSOR_NEURON_HOST_L_IDX], sensory_input[SENSOR_NEURON_HOST_R_IDX]));
        acetylcholine_level = fmin(1.0f, max_sensory_input * 1.5f);
        
        float conflicting_signals = fabs(sensory_input[SENSOR_NEURON_DIST_IDX] - (sensory_input[SENSOR_NEURON_HOST_L_IDX] + sensory_input[SENSOR_NEURON_HOST_R_IDX]) / 2.0f);
        gaba_level = fmin(1.0f, conflicting_signals);

        if (sensory_input[SENSOR_NEURON_DIST_IDX] > 0.9f) {
            epinephrine_level = 1.0f;
            cortisol_level = fmin(1.0f, cortisol_level + 0.2f);
        } else {
            epinephrine_level *= 0.99f;
            cortisol_level *= 0.995f;
        }

        if (max_sensory_input > 0.5f) {
            norepinephrine_level = fmin(1.0f, norepinephrine_level + 0.1f);
            glutamate_level = fmin(1.0f, glutamate_level + 0.1f);
        } else {
            norepinephrine_level *= 0.99f;
            glutamate_level *= 0.99f;
        }
        
        bool getting_closer_to_obstacle = (sensory_input[SENSOR_NEURON_DIST_IDX] > prev_dist_input + 0.01f);
        bool getting_closer_to_host = (sensory_input[SENSOR_NEURON_HOST_L_IDX] > prev_host_input_l + 0.01f &&
                                      sensory_input[SENSOR_NEURON_HOST_R_IDX] > prev_host_input_r + 0.01f);
        bool getting_further_from_host = (sensory_input[SENSOR_NEURON_HOST_L_IDX] < prev_host_input_l - 0.01f &&
                                        sensory_input[SENSOR_NEURON_HOST_R_IDX] < prev_host_input_r - 0.01f);

        float actual_reward = 0.0f;
        if (getting_closer_to_host) {
            actual_reward = 1.0f;
            endorphin_level = fmin(1.0f, endorphin_level + 0.1f);
            oxytocin_level = fmin(1.0f, oxytocin_level + 0.1f);
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
            serotonin_level = fmin(1.0f, serotonin_level + 0.01f);
        }
        
        float hormonal_flux = fabs(dopamine_level) + fabs(serotonin_level);
        if (hormonal_flux < 0.2f && actual_reward >= 0.0f) {
            stability_level = fmin(1.0f, stability_level + 0.05f);
        } else {
            stability_level = fmax(0.0f, stability_level - 0.1f);
        }

        NeuralNet_step(sensory_input, motor_output);
        
        float final_left_output = motor_output[0];
        float final_right_output = motor_output[1];

        if (epinephrine_level > 0.5f) {
            final_left_output = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
            final_right_output = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
        }
        
        float final_motor_output[2] = {final_left_output, final_right_output};
        send_motor_outputs(final_motor_output);

        usleep(100000);
    }

    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    close(motor);
    close(sr04_l);
    return 0;
}
