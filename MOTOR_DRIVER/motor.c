#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include "../common/motor/ioctl_car_cmd.h"

#define BUFSIZE                2 // Defines Read Buffer

#define MAX_SPEED            0x63
#define MID_SPEED            0x4F
#define MIN_SPEED            0x00

#define I2C_BUS_AVAILABLE 1  
#define SLAVE_DEV_NAME     ("CARMOTOR"   )
#define MOTOR_SLAVE_ADDR (  0x16   )


static struct i2c_adapter * motorI2CAdapter = NULL;
static struct i2c_client * motorI2CClient = NULL;

char LEFT[5] =       { 0x01, 0x00, MID_SPEED, 0x01, MID_SPEED };

char RIGHT[5] =      { 0x01, 0x01, MID_SPEED, 0x00, MID_SPEED };

char FORWARD[5] = { 0x01, 0x01, MAX_SPEED, 0x01, MAX_SPEED };

char FORWARD_SLOW[5] = { 0x01, 0x01, MID_SPEED, 0x01, MID_SPEED };

char BACKWARD[5] = { 0x01, 0x00, MAX_SPEED, 0x00, MAX_SPEED };

char STOP[5] =      { 0x01, 0x00,    0x00, 0x00,    0x00   };


dev_t        dev  = 0;
_Bool        flag = 1;

static struct     class * devClass;
static struct     cdev myCharDevice;

static int __init DeviceInit(void);
static void __exit DeviceExit(void);

// Device Functions
static int DeviceOpen(struct inode * inode, struct file * file);
static int DeviceRelease(struct inode * inode, struct file * file);

static void Forward(void);
static void ForwardSlow(void);
static void Backward(void);
static void Left   (void);
static void Right  (void);

static long chardevIoctl(struct file *, unsigned int, unsigned long);

int motor_write(unsigned char * buf, unsigned int len);

// Match File Operation Functions into Structure
static struct file_operations fOpStruct = {
    .owner = THIS_MODULE,
    .read  = NULL,
    .write = NULL,
    .open  = DeviceOpen,
    .release = DeviceRelease,
    .unlocked_ioctl = chardevIoctl
};

static int DeviceOpen(struct inode * inode, struct file * file)
{
    printk("Device File Opened \n ");
    return 0;
}

static int DeviceRelease(struct inode * inode, struct file * file)
{
    printk("Device File Closed \n") ;
    return 0;
}

static const struct i2c_device_id motorID [] = {
    {    SLAVE_DEV_NAME,    0    },
    {                            }
};

MODULE_DEVICE_TABLE(i2c, motorID);

static int motor_probe(struct i2c_client *client) {
    printk("Probing motor device at 0x%02x\n", client->addr);
    return 0;
}

static void remove_motor(struct i2c_client *client) {
    printk("Removing motor device at 0x%02x\n", client->addr);
}


static struct i2c_driver motorI2CDriver = {

    .driver = {
        .name = SLAVE_DEV_NAME,
        .owner = THIS_MODULE,
    },
    .probe    = motor_probe,
    .remove    = remove_motor,
    .id_table = motorID,
};


static struct i2c_board_info MOTOR_INFO = {

    I2C_BOARD_INFO(SLAVE_DEV_NAME, MOTOR_SLAVE_ADDR)

};

static int __init DeviceInit(void)
{

    int major = MAJOR(dev);
    int minor = MINOR(dev);
    dev = MKDEV(major,minor);

    if((alloc_chrdev_region(&dev, minor, 1, "i2cmotor")) < 0)  {
        printk (KERN_INFO "ERROR : cannot allocate major number");
        return -1;
    }

    printk("Major = %d , Minor = %d \n", MAJOR(dev), MINOR(dev));

    cdev_init(&myCharDevice, &fOpStruct);

    if(cdev_add(& myCharDevice, dev, 1) < 0) {
        printk(KERN_INFO "ERROR : cannot add the device");
        goto r_class;
    }

    if((devClass = class_create("i2cmotorClass")) == NULL) {
        printk(KERN_INFO "ERROR : cannot create the struct class");
        goto r_class;
    }

    if((device_create(devClass, NULL, dev, NULL, "motor")) == NULL) {
        printk("ERROR : CANNOT CREATE THE DEVICE");
        goto r_device;
    }

    motorI2CAdapter    =    i2c_get_adapter   (I2C_BUS_AVAILABLE);

    if(motorI2CAdapter == NULL) {

        class_destroy   (devClass)    ;
        unregister_chrdev_region(dev,1);
        cdev_del     (& myCharDevice);
        return -1;

    }

    motorI2CClient = i2c_new_client_device(motorI2CAdapter, & MOTOR_INFO);

    if(motorI2CClient == NULL) {
        class_destroy   (devClass)    ;
        unregister_chrdev_region(dev,1);
        cdev_del     (& myCharDevice);
        return -1;
    }

    i2c_add_driver(& motorI2CDriver);

    printk(KERN_INFO "motor is ready");


    return 0;

r_class:
    class_destroy   (devClass)    ;

r_device:
    unregister_chrdev_region(dev,1);
    cdev_del     (& myCharDevice);

    return -1;
}



static void __exit DeviceExit()
{


    i2c_unregister_device(motorI2CClient);

    i2c_del_driver(& motorI2CDriver);
    device_destroy    (devClass, dev);
    class_destroy    (devClass);
    cdev_del        (& myCharDevice);

    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "Devices are released");


    printk    ("Device Driver Remove : Success");
}


static void Forward(void)

{

    motor_write(FORWARD, 5);

}

static void ForwardSlow(void) {
    motor_write(FORWARD_SLOW, 5);
}

static void Backward(void)
{
    motor_write(BACKWARD, 5);
}

// FAST TRUN LEFT
static void Left(void)
{
    motor_write(LEFT, 5);
}

static void Right(void)
{
    motor_write(RIGHT, 5);
}

static void Stop(void)
{
    motor_write(STOP, 5);
}

static long chardevIoctl(struct file * file, unsigned int command, unsigned long arg)
{
    switch(command) {
    case PI_CMD_LEFT    :
        Left();
        printk(KERN_INFO "COMMAND: left");
        break;
    case PI_CMD_RIGHT   :
        Right();
        printk(KERN_INFO "COMMAND: right");
        break;
    case PI_CMD_FORWARD :
        Forward();
        printk(KERN_INFO "COMMAND: forward");
        break;
    case PI_CMD_FORWARD_SLOW :
        ForwardSlow();
        printk(KERN_INFO "COMMAND: forward_slow");
        break;
    case PI_CMD_BACKWARD:
        Backward();
        printk(KERN_INFO "COMMAND: backward");
        break;
    case PI_CMD_STOP :
        Stop();
        printk(KERN_INFO "COMMAND: stop");
        break;
    case PI_CMD_IO :
        struct ioctl_info info;
        int ret = copy_from_user(&info, (struct ioctl_info *)arg, sizeof(info));
        printk(KERN_INFO "COMMAND: io");
        if (ret < 0) {
            Stop();
        } else {
            motor_write(info.buf, 5);
        }
        break;
    }
    return command;
}

int motor_write(unsigned char * buf, unsigned int len)
{
    int ret = i2c_master_send(motorI2CClient, buf, 5);
    return ret;
}

module_init(DeviceInit);
module_exit(DeviceExit);

MODULE_LICENSE("GPL");



