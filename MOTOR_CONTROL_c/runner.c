#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#define DEVNAME "/dev/motor"
#define SR04 "/dev/sr04"
#define IR "/dev/ir_device"
#include "../common/motor/ioctl_car_cmd.h"
#define AVOID_DIST 30
#define BACK_DIST  10
#define MOVE_TIME         1
#define SLEEP_TIME        60
#define ONE_MILI_SEC      1000
int motor;
int sr04;
int ir;
void sigHandler (int dummy)
{
    close (sr04);
    close (ir);
    ioctl(motor, PI_CMD_STOP, sizeof (struct ioctl_info));
    close (motor);
    exit (0);
}
int main ()
{
    u_int32_t dist=0, irLeft =0, irRight=0, prev_dist = 0;
    puts ("Runner begins");
    motor = open (DEVNAME, O_RDWR);
    ir = open (IR, O_RDWR);
    sr04 = open (SR04, O_RDWR);
    signal (SIGINT, sigHandler);
    while (true) {
        prev_dist = dist;
        char buf[8];
        char ir_flag[5];
        read (sr04, buf, 8);
        dist = atoi(buf);
        read (ir, ir_flag, 5);
        
        printf("distance is %d\n", dist);
        if (dist < AVOID_DIST) {
            if (ir_flag[0] == 'B' && (dist < BACK_DIST)) {
                ioctl(motor, PI_CMD_BACKWARD, sizeof (struct ioctl_info));
                printf("backward --> (%u)\n", dist);
            } else if (ir_flag[0] == 'L') {
                ioctl(motor, PI_CMD_RIGHT, sizeof (struct ioctl_info));
                printf("right --> (%u)\n", dist);
            } else if (ir_flag[0] == 'R') {
                ioctl(motor, PI_CMD_LEFT, sizeof (struct ioctl_info));
                printf("left --> (%u)\n", dist);
            } else if (!dist) {
                ioctl(motor, PI_CMD_FORWARD);
            } else {
                ioctl(motor, PI_CMD_LEFT, sizeof (struct ioctl_info));
                printf("left (predefined) --> (%u)\n", dist);
            }
        } else {
            static int counter;
            if (ir_flag[0] == 'B' && dist < BACK_DIST) {
                ioctl(motor, PI_CMD_BACKWARD, sizeof (struct ioctl_info));
                printf("backward --> (%u)\n", dist);
            } else if (ir_flag[0] == 'R') {
                ioctl(motor, PI_CMD_LEFT, sizeof (struct ioctl_info));
                printf("left --> (%u)\n", dist);
            } else if (ir_flag[0] == 'L') {
                ioctl(motor, PI_CMD_RIGHT, sizeof (struct ioctl_info));
                printf("right --> (%u)\n", dist);
            } else {
                ioctl(motor, PI_CMD_FORWARD, sizeof (struct ioctl_info));
                printf("forward --> (%u)\n", dist);
            }
        }
	usleep(ONE_MILI_SEC * SLEEP_TIME);
    }
    return 0;
}

