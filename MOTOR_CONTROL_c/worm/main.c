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

#define DEVNAME "/dev/motor"
#define SR04 "/dev/sr04"

#define SENSOR_NEURON_DIST_IDX 0
#define MOTOR_NEURON_LEFT_IDX 1
#define MOTOR_NEURON_RIGHT_IDX 2

#define MOTOR_SPEED_MAX 100
#define SENSOR_DIST_MAX_CM 500

int motor;
int sr04;
int turn_direction = -1; // -1: no turn, 0: left, 1: right

void sigHandler(int dummy)
{
    puts("Exiting...");
    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    close(sr04);
    close(motor);
    exit(0);
}

void read_sensors(float* sensory_input) {
    char dist_buf[8];
    u_int32_t dist;

    read(sr04, dist_buf, sizeof(dist_buf));
    dist = atoi(dist_buf);

    if (dist > 0 && dist < SENSOR_DIST_MAX_CM) {
        sensory_input[SENSOR_NEURON_DIST_IDX] = 1.0f - ((float)dist / SENSOR_DIST_MAX_CM);
    } else {
        sensory_input[SENSOR_NEURON_DIST_IDX] = 0.0f;
    }
}

void send_motor_outputs(float* motor_output) {
    struct ioctl_info io;
    io.size = 5;
    
    int straight_speed = 70;
    int turn_speed = 70; // Turning speed for zero-point turn
    int left_speed, right_speed;

    float obstacle_strength = fmax(fabs(motor_output[MOTOR_NEURON_LEFT_IDX]), fabs(motor_output[MOTOR_NEURON_RIGHT_IDX]));
    
    if (obstacle_strength > 0.9) {
        // If no turn direction is set, choose one randomly
        if (turn_direction == -1) {
            turn_direction = rand() % 2; // 0 for left, 1 for right
        }
        
        if (turn_direction == 0) { // Left turn (left motor reverse, right motor forward)
            left_speed = -turn_speed; 
            right_speed = turn_speed;
        } else { // Right turn (left motor forward, right motor reverse)
            left_speed = turn_speed;
            right_speed = -turn_speed;
        }
    } else {
        // No obstacle detected, reset turn direction and move straight
        turn_direction = -1;
        left_speed = straight_speed;
        right_speed = straight_speed;
    }

    // Ensure speeds are within a valid range [0, MOTOR_SPEED_MAX]
    if (left_speed < -MOTOR_SPEED_MAX) left_speed = -MOTOR_SPEED_MAX;
    if (right_speed < -MOTOR_SPEED_MAX) right_speed = -MOTOR_SPEED_MAX;
    if (left_speed > MOTOR_SPEED_MAX) left_speed = MOTOR_SPEED_MAX;
    if (right_speed > MOTOR_SPEED_MAX) right_speed = MOTOR_SPEED_MAX;

    io.buf[0] = 1; 
    io.buf[1] = (left_speed > 0); 
    io.buf[2] = abs(left_speed); 
    io.buf[3] = (right_speed > 0);
    io.buf[4] = abs(right_speed); 

    ioctl(motor, PI_CMD_IO, &io);
    printf("Obstacle: %.2f | Turn: %s | Speeds: L: %d, R: %d\n",
           obstacle_strength,
           (turn_direction == 0 ? "Left" : (turn_direction == 1 ? "Right" : "None")),
           left_speed, right_speed);
}

int main(void) {
    motor = open(DEVNAME, O_RDWR);
    if (motor < 0) {
        perror("Failed to open motor device");
        return -1;
    }
    sr04 = open(SR04, O_RDWR);
    if (sr04 < 0) {
        perror("Failed to open sr04 device");
        close(motor);
        return -1;
    }

    signal(SIGINT, sigHandler);
    
    srand(time(NULL)); 

    NeuralNet_init();

    float sensory_input[MAX_NEURONS] = {0};
    float motor_output[MAX_NEURONS] = {0};

    puts("Neural network control loop started.");

    while (true) {
        read_sensors(sensory_input);
        NeuralNet_step(sensory_input, motor_output);
        send_motor_outputs(motor_output);
        usleep(100000);
    }

    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    close(motor);
    close(sr04);

    return 0;
}
