#include<iostream>
#include<sys/types.h>
#include<cnetdb>
#include<cunistd>
#include<raspicam/raspicam.h>
#include<cstring>
#include<sys/ioctl.h>
#include<fstream>
#include<vector>
typedef unsigned char uchar;
class RaspiCarRunner {
    int motor;
    int sr04;
    int ir;
    raspicam::RaspiCam camera;
    void signalHandler(int dummy) {
        closeDevice();
        exit(0);
    }
    int openDevice() {
        camera.setWidth(480);
        camera.setWidth(320);
        camera.setFormat(raspicam::RASPICAM_FORMAT_RGB);
        camera.open();
        if (!camera.isOpened()) {
            std::cerr << "Error: cannot open camera." << std::endl;
            return EPERM;
        }
        std::vector<uchar> imageData;
        std::cout << "Waiting for camera for a second..." << std::endl;
        sleep(1);
        camera.grab();
        motor = open(DEVNAME, O_RDWR);
        ir = open(IR, O_RDWR);
        sr04 = open(SR04, O_RDWR);
    }
    void closeDevice() {
        ioctl(motor, PI_CMD_STOP, sizeof(struct ioctl_info));
        close(motor);
        close(sr04);
        close(ir);
        camera.release();
    }
    int handleCamera() {
        camera.retrieve(imageData);
        if(imageData.empty()) {
            std::cerr << "Error: failed to capture image." << std::endl;
            camera.release();
            return -ENODATA;
        }
    
        std::ofstream capturedImage("shot.jpg". std::ios::binary);
        if(capturedImage.is_open()) {
            capturedImage.write(reinterpret_cast<const char*>(imageData.data(), imageData.size());
            outputFile.close();
            std::cout << "saved image file to jpg." << std::endl;
        } else {
            std::cout << "cannot open file for writing" << std::endl;
            return -EPERM;
        }
        return 0;
    }
    void doIoctl(const char *cmd) {
        struct ioctl_info data;
        data.size = 5;
        memcpy(data, cmd, sizeof(char)*data_size);
        ioctl(motor, PI_CMD_IO, data);
    }
}

class DataRetriever {
    RaspiCarRunner runner;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, j;
    size_t len;
    ssize_t nread;
    bool    ir_requested;
    char buf[5];
    int setupUDPSock() {
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;
        s = getaddrinfo("hobbies.yoonjin2.kr", "63000", &hints, &servinfo);
        if(s) {
            std::cerr << "getaddrinfo: " << gai_strerror(s) << std::endl;
            return -EAGAIN;
        }
        for(rp = result; rp != NULL; rp = rp->ai_next) {
            sfd = socket(rp->ai_faimly, rp->ai_socktype, rp->ai_protocol);
            if(sfd == -1) {
                continue;
            }
            if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
                break;
            }
            close(sfd);
        } 
        if(rp == NULL) {
            std::cerr << "Could not connect to destination\n" << std::endl;
            return -ECONNREFUSED;
        }
        freeaddrinfo(result);
        return 0;
    }
    int setupSender() {
        int udpSetupError = setupUDPSock();
        if(udpSetupError) {
            exit(udpSetupError);
            runner.openDevice(); //PLEASE DEVELOP FROM HERE
        }
    }
    int sendData() {
        char distance[16];

        char msg[32];
        read(sr04, &distance, sizeof(distance));
        if(ir_requested) {
            write(ir, "ON", sizeof(char) * 2);
        } else {
            write(ir, "OFF", sizeof(char) * 3);
            //if ir turned off, always NONE
        }
        char ir_value[5];
        read(ir, &ir_value, sizeof(ir_value));
        strcat(msg, distance);
        strcat(msg, ";");
        strcat(msg, ir_value);
        write(sfd, msg, sizeof(msg));
    }
    int get_data() {
        char cmd[5];
        read(sfd, cmd, sizeof(cmd));
    }
}
