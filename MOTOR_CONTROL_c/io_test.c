#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define DEVNAME "/dev/motor"
#include "../common/motor/ioctl_car_cmd.h"

int main ()
{

    int dev ;

    dev = open ( DEVNAME, O_RDWR ) ;

    if ( dev < 0 ) exit (1);

    struct ioctl_info test = {
        .buf = { 0x01, 0x00, 0x00, 0x00, 0x00 },
        .size = 5
    };
    ioctl ( dev, PI_CMD_IO, &test) ;

    close (dev);

    return 0 ;
}
