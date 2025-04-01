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
#define AVOID_DIST 50
#define DIST_MAX   300
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
        struct ioctl_info io;
        io.buf[0] = 1;
        io.size = 5;
        if(dist <= 57 && dist >= 30) {
            io.buf[2] = io.buf[4] = dist + dist / 2 + dist / 4;
        } else if (dist > 200 && dist < DIST_MAX) {
            io.buf[2] = io.buf[4] = 80;

        } else if(dist > 57 && dist < 100) {
            io.buf[2] = io.buf[4] = dist / 2 + dist / 4; 
            
        } else {
            io.buf[2] = io.buf[4] = 30;
        }
        printf("distance is %d\n", dist);
recheck:
        if (dist < AVOID_DIST) {
            if (dist < BACK_DIST) {
                io.buf[1] = 0;
                io.buf[3] = 0;
                ioctl(motor, PI_CMD_IO, &io, &io);
                printf("backward --> (%u)\n", dist);
                while(dist < BACK_DIST) {
                    read (sr04, buf, 8);
                    dist = atoi(buf);
                    goto recheck;
                }
            } else if (ir_flag[0] == 'L') {
                io.buf[1] = 0;
                io.buf[3] = 1;
                ioctl(motor, PI_CMD_IO, &io);
                printf("right --> (%u)\n", dist);
                while(dist < AVOID_DIST) {
                    read (sr04, buf, 8);
                    dist = atoi(buf);
                    goto recheck;
                }
            } else if (ir_flag[0] == 'R') {
                io.buf[1] = 1;
                io.buf[3] = 0;
                ioctl(motor, PI_CMD_IO, &io);
                printf("left --> (%u)\n", dist);
                while(dist < AVOID_DIST) {
                    read (sr04, buf, 8);
                    dist = atoi(buf);
                    goto recheck;
                }
            } else if (!dist) {
                ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
            } else {
                io.buf[1] = 0;
                io.buf[3] = 1;
                ioctl(motor, PI_CMD_IO, &io);
                printf("left (predefined) --> (%u)\n", dist);
                while(dist < AVOID_DIST) {
                    read (sr04, buf, 8);
                    dist = atoi(buf);
                    goto recheck;
                }
            }
        } else {
                io.buf[1] = 1;
                io.buf[3] = 1;
                ioctl(motor, PI_CMD_IO, &io);
            printf("forward --> (%u)\n", dist);
            if (ir_flag[0] == 'R') {
                io.buf[1] = 1;
                io.buf[3] = 0;
                ioctl(motor, PI_CMD_IO, &io);
                printf("left --> (%u)\n", dist);
                while(dist < AVOID_DIST) {
                    read (sr04, buf, 8);
                    dist = atoi(buf);
                    goto recheck;
                }
            }
        }
	usleep(ONE_MILI_SEC * SLEEP_TIME);
    }
    return 0;
}

