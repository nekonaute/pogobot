# Pogobot

## Overview 
The pogobot is a small robot designed for swarm robotics experiments, targeted at collective behaviour and social learning research.
The robot is composed of a head (above) with a iCE40UP5K FPGA chip including a softcore processor, an IMU, fast Infra-Red communication devices and a LED.
The head is plugged on a belly (under) which composed of even more LEDs, the motor controler, the battery and the battery regulation system.

To help the understanding, there is two version names used inside the project, one for the hardware and one for the software. <br> 
The following table gives an overview :

| hardware version | software version | comments                                           |
|:----------------:|:----------------:|----------------------------------------------------|
|        V3        |        V1        | initial version                                    |
|        V3        |        V2        | reduce communication speed to increase reliability |
|        V3        |       V2.1       | reduce header footprint                            |
|        V3        |       V2.2       | introduce short message and clean the pogobios     |
|        V3        |      V2.2.1      | fix the IR programation problem                    |
|        V3        |       V2.3       | API standardization : motor                        |
|        V3        |       V2.4       | API standardization : IR messaging                 |

## Git description
This project is divided into 3 folders :
- Hardware
- Software
- Tools

Hardware contains the schematics and other files to build your own robot from scratch. 

All the schematics were done using KiCad 6. You can find : 
- 'Belly' folder with the belly schematics
- 'Head' folder with the head schematics
- '3d-addons' folder contains all the 3D parts to print to complete a robot
- '3Dmodels' and other folders regroups all the libraries, 3D models and foorprints for KiCad.
   
Software contains the gateware / software / SDK builder in expert mode to upload inside the robot.
It exists 2 softwares inside the robot. 
- The bootloader is the one called when you boot the robot. It contains a gateware and the pogobios.
- The user space is called by a command inside the bootloader. It contains your code and a gateware. <br>
/!\ If you plan to use the pogobot in the user space please use the sdk [here](https://github.com/nekonaute/pogobot-sdk).
  
Tools corresponds to the differents tools used around the robot.

### Shortcuts to

- [Normal mode software](https://github.com/nekonaute/pogobot-sdk)
- [Expert mode software](/Software/readme.md)
- [Hardware](/Hardware/readme.md)
- [Ir Remote software](/readme-irRemote.md)
- [Charger](/Hardware/readme.md)
- [Pogobject](/Software/pogobject/readme.md)
- [PogoWallApp](/Software/pogoWallApp/readme.md)


## Pogobios LED Status

<img src="Images/pogobot_led_status.png" alt="pogobios leds status" width="800"/>






