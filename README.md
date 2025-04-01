# Motor Control System for Yahboom AI Vision Robot Car

## Abstract
This project implements an intelligent motor control system for the 
Yahboom Raspbot AI Vision Robot Car. The system utilizes GPIO pins to 
monitor input signals and controls the robot's movement based on sensor 
data and predefined logic.

## Features
  - Provides real-time feedback on hardware functionality

- **Obstacle Detection**:
  - Uses infrared sensors for obstacle detection
  - Implements safety protocols to avoid collisions

- **Motor Control Algorithm**:
  - Adjusts motor speed based on distance from obstacles
  - Handles directional control using sensor inputs

- **Automatic Controller**:
  - Supports both automatic and driving mode

## Setup Instructions
### Required Hardware
- Yahboom Raspbot AI Vision Robot Car (Raspberry Pi 4 version)
- GPIO pins configured according to the robot's hardware specifications

### Software Requirements
- Raspberry Pi OS
- C programming language for implementation
- Necessary libraries for GPIO control and sensor input processing

## Motor Control Logic Overview
The motor control system operates based on the following logic:
1. **Distance Monitoring**: 
   - Continuously reads distance data from sensors
   - Adjusts motor speed to maintain safe distance from obstacles

2. **Directional Control**:
   - Uses infrared sensors for obstacle detection
   - Implements turn logic when obstacles are detected in path

3. **Error Handling**:
   - Detects and reports hardware malfunctions
   - Includes safeguards against system failures

## Usage Instructions
1. Clone the repository to your Raspberry Pi.
2. Install required dependencies.
3. Run the motor control script with appropriate arguments.

Example command:
```bash
cd MOTOR_CONTROL_c
./runner
```

## Known Issues
- **Hardware Limitation**: 
  - Left infrared sensor is less reliable due to hardware misfunction
  - Right infrared sensor is more accurate and should be trusted

## Conclusion
This project demonstrates a robust solution for controlling a robotic 
system using GPIO pins and sensor data. It highlights the importance of 
thorough hardware testing and adaptive control algorithms in embedded 
systems development.

## Additional Info
  - **GPIO Pin Map**: 
  - Checks pin map via `/sys/kernel/debug/gpio`


