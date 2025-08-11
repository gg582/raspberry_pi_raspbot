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

// Device file paths for motor driver and ultrasonic sensor
#define DEVNAME "/dev/motor"
#define SR04 "/dev/sr04"

// Indices for sensory and motor neurons
#define SENSOR_NEURON_DIST_IDX 0
#define MOTOR_NEURON_LEFT_IDX 1
#define MOTOR_NEURON_RIGHT_IDX 2

// Maximum motor speed and sensor distance (cm)
#define MOTOR_SPEED_MAX 100
#define SENSOR_DIST_MAX_CM 500

// File descriptors for devices
int motor;
int sr04;

// Direction of turn: -1 = no turn, 0 = left, 1 = right
int turn_direction = -1;

/*
 * Signal handler for graceful exit on Ctrl+C
 */
void sigHandler(int dummy)
{
    puts("Exiting...");
    // Stop motors immediately
    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    // Close device file descriptors
    close(sr04);
    close(motor);
    // Exit program
    exit(0);
}

/*
 * Read sensors and fill sensory input array
 * sensory_input: array to fill with normalized sensor data
 */
void read_sensors(float* sensory_input) {
    char dist_buf[8];
    u_int32_t dist;

    // Read raw distance data from ultrasonic sensor device
    read(sr04, dist_buf, sizeof(dist_buf));
    dist = atoi(dist_buf); // Convert string to integer distance (cm)

    // Normalize distance: closer obstacle => higher input (1 to 0)
    if (dist > 0 && dist < SENSOR_DIST_MAX_CM) {
        sensory_input[SENSOR_NEURON_DIST_IDX] = 1.0f - ((float)dist / SENSOR_DIST_MAX_CM);
    } else {
        // If no valid reading, set input to zero
        sensory_input[SENSOR_NEURON_DIST_IDX] = 0.0f;
    }
}

/*
 * Send motor commands to the motor device using motor outputs
 * motor_output: activations of motor neurons to translate to speeds
 */
void send_motor_outputs(float* motor_output) {
    struct ioctl_info io;
    io.size = 5;
    
    // Define base speeds for straight and turning
    int straight_speed = 70;
    int turn_speed = 70; // Speed used for in-place turn
    int left_speed, right_speed;

    // Calculate maximum absolute motor activation as obstacle strength
    float obstacle_strength = fmax(fabs(motor_output[MOTOR_NEURON_LEFT_IDX]), fabs(motor_output[MOTOR_NEURON_RIGHT_IDX]));
    
    // If obstacle detected strongly (> 0.9), initiate turn
    if (obstacle_strength > 0.9) {
        // If no turn direction chosen yet, pick one randomly
        if (turn_direction == -1) {
            turn_direction = rand() % 2; // 0 = left turn, 1 = right turn
        }
        
        if (turn_direction == 0) {
            // Left turn: left motor backward, right motor forward
            left_speed = -turn_speed; 
            right_speed = turn_speed;
        } else {
            // Right turn: left motor forward, right motor backward
            left_speed = turn_speed;
            right_speed = -turn_speed;
        }
    } else {
        // No obstacle detected: move straight and reset turn direction
        turn_direction = -1;
        left_speed = straight_speed;
        right_speed = straight_speed;
    }

    // Clamp speeds to valid range [-MOTOR_SPEED_MAX, MOTOR_SPEED_MAX]
    if (left_speed < -MOTOR_SPEED_MAX) left_speed = -MOTOR_SPEED_MAX;
    if (right_speed < -MOTOR_SPEED_MAX) right_speed = -MOTOR_SPEED_MAX;
    if (left_speed > MOTOR_SPEED_MAX) left_speed = MOTOR_SPEED_MAX;
    if (right_speed > MOTOR_SPEED_MAX) right_speed = MOTOR_SPEED_MAX;

    // Prepare ioctl buffer:
    // io.buf[0]: always 1 (some command flag)
    // io.buf[1]: left motor direction (1 = forward, 0 = backward)
    // io.buf[2]: left motor speed (absolute)
    // io.buf[3]: right motor direction (1 = forward, 0 = backward)
    // io.buf[4]: right motor speed (absolute)
    io.buf[0] = 1; 
    io.buf[1] = (left_speed > 0); 
    io.buf[2] = abs(left_speed); 
    io.buf[3] = (right_speed > 0);
    io.buf[4] = abs(right_speed); 

    // Send motor command via ioctl to motor device driver
    ioctl(motor, PI_CMD_IO, &io);

    // Print debug info for obstacle strength, turn direction, and speeds
    printf("Obstacle: %.2f | Turn: %s | Speeds: L: %d, R: %d\n",
           obstacle_strength,
           (turn_direction == 0 ? "Left" : (turn_direction == 1 ? "Right" : "None")),
           left_speed, right_speed);
}

int main(void) {
    // Open motor device
    motor = open(DEVNAME, O_RDWR);
    if (motor < 0) {
        perror("Failed to open motor device");
        return -1;
    }

    // Open ultrasonic sensor device
    sr04 = open(SR04, O_RDWR);
    if (sr04 < 0) {
        perror("Failed to open sr04 device");
        close(motor);
        return -1;
    }

    // Register signal handler for Ctrl+C
    signal(SIGINT, sigHandler);
    
    // Seed random number generator for turn direction choice
    srand(time(NULL)); 

    // Initialize neural network
    NeuralNet_init();

    // Buffers for sensory inputs and motor outputs
    float sensory_input[MAX_NEURONS] = {0};
    float motor_output[MAX_NEURONS] = {0};

    puts("Neural network control loop started.");

    // Control loop: read sensors, step neural net, send motor commands
    while (true) {
        read_sensors(sensory_input);
        NeuralNet_step(sensory_input, motor_output);
        send_motor_outputs(motor_output);
        // Sleep 100ms between iterations
        usleep(100000);
    }

    // Stop motors and close devices on exit (never reached normally)
    ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
    close(motor);
    close(sr04);

    return 0;
}

