import time
import socket
from YB_Pcb_Car import YB_Pcb_Car
import struct
def read_until_null(device_path):
    """
    Reads data from the specified device file until a null byte (0x00) is encountered.

    Args:
        device_path: Path to the device file.

    Returns:
        A bytes object containing the data read from the device,
        excluding the null byte.
    """
    try:
        with open(device_path, "rb") as dev:
            data = bytearray()    # Use bytearray for efficient appending
            while True:
                byte = dev.read(1)    # Read one byte at a time
                if not byte:    # Check for end-of-file
                    break
                if byte == b'\x00':    # Check for null byte
                    break
                data.extend(byte)    # Append byte to the data

        return bytes(data)    # Convert bytearray to bytes

    except FileNotFoundError:
        print(f"Error: File not found at '{device_path}'")
        return None
    except Exception as e:
        print(f"An error occurred while reading from '{device_path}': {e}")
        return None

def main():
    car = YB_Pcb_Car()

#    UDP_IP = "192.168.0.2"    # Replace with the IP address of your computer
#    UDP_PORT = 5005
#    sock = socket.socket(socket.AF_INET, # Internet
#                                             socket.SOCK_DGRAM) # UDP
#    sock.bind(("0.0.0.0",UDP_PORT))
    while True:
#        data, addr = sock.recvfrom(2)
#        if data != "ok":
#                break
        """Reads data from SR04 and IR sensors and prints raw values."""

        sr_data = read_until_null("/dev/sr04")
        ir_data = read_until_null("/dev/ir_device")

        if sr_data is not None:
            # Process SR04 data (decode or interpret based on sensor's output format)
            print(sr_data.decode())
            if int(sr_data.decode()) > 60 and ir_data.decode() == "BOTH":
                car.Car_Run(sr_data,sr_data)
            else:
                if ir_data.decode() == "RIGHT":
                    while int(sr_data.decode()) < 60 and ir_data.decode() != "BOTH":
                        sr_data = read_until_null("/dev/sr04")
                        ir_data = read_until_null("/dev/ir_device")
                        car.Car_Spin_Left(50,150)
                if ir_data.decode() == "LEFT":
                    while int(sr_data.decode()) < 60 and ir_data.decode() != "BOTH":
                        sr_data = read_until_null("/dev/sr04")
                        ir_data = read_until_null("/dev/ir_device")
                        car.Car_Spin_Right(150,50)
                if int(sr_data.decode()) > 60:
                    while ir_data != "BOTH":
                        sr_data = read_until_null("/dev/sr04")
                        ir_data = read_until_null("/dev/ir_device")
                        car.Car_Spin_Right(150,50)
                else:
                    while int(sr_data.decode()) < 60:
                        sr_data = read_until_null("/dev/sr04")
                        ir_data = read_until_null("/dev/ir_device")
                        car.Car_Spin_Right(150,50)
        else:
            print("SR04 data unavailable")

        if ir_data is not None:
            # Process IR data (decode or interpret based on sensor's output format)
            print(ir_data.decode())
        else:
            print("IR data unavailable")
        time.sleep(0.00005)
    car.Car_Stop()
#    sock.close()

if __name__ == "__main__":
    main()

