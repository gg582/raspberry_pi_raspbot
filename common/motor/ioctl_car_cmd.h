#ifndef IOCTL_CAR_CMD_H
#define IOCTL_CAR_CMD_H

#include <linux/ioctl.h>

struct ioctl_info {
    unsigned long size;
    char buf[5];
};

enum {
    CMD_LEFT = 3,
    CMD_RIGHT,
    CMD_FORWARD,
    CMD_FORWARD_SLOW,
    CMD_BACKWARD,
    CMD_STOP,
    CMD_IO,
};

#define			IOCTL_MAGIC     'G'

#define			PI_CMD_LEFT		_IOW(IOCTL_MAGIC, CMD_LEFT,	struct ioctl_info)
#define			PI_CMD_RIGHT		_IOW(IOCTL_MAGIC, CMD_RIGHT,	struct ioctl_info)
#define			PI_CMD_FORWARD		_IOW(IOCTL_MAGIC, CMD_FORWARD,  struct ioctl_info)
#define			PI_CMD_FORWARD_SLOW		_IOW(IOCTL_MAGIC, CMD_FORWARD_SLOW,  struct ioctl_info)
#define			PI_CMD_BACKWARD		_IOW(IOCTL_MAGIC, CMD_BACKWARD, struct ioctl_info)
#define			PI_CMD_STOP		_IOW(IOCTL_MAGIC, CMD_STOP,	struct ioctl_info)
#define			PI_CMD_IO		_IOW(IOCTL_MAGIC, CMD_IO,	struct ioctl_info)

#endif
