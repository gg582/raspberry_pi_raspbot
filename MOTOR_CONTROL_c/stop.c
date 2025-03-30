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

    if ( dev < 0 ) {
        fprintf(stderr, "motor device is not working, error code %d\n", dev);
    	exit (1);
    }

    ioctl ( dev, PI_CMD_STOP) ;

    close (dev);

    return 0 ;
}
