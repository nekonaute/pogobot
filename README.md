# Pogobot

## Overview 
The pogobot is a small robot designed for swarm robotics experiments, targeted at collective behaviour and social learning research.
The robot is composed of a head (above) with a iCE40UP5K FPGA chip including a softcore processor, an IMU, fast Infra-Red communication devices and a LED.
The head is plugged on a belly (under) which composed of even more LEDs, the motor controler, the battery and the battery regulation system.


## Git description
This project is divided into 5 folders :

- 3d-addons
- docs
- Hardware
- Software
- Scripts

3d-addons contains the 3d printable mesh necessary to assemble a complete robot. 

Hardware contains the schematics and other files to build your own robot from scratch. 

All the schematics were done using KiCad 6. You can find : 
- 'Belly' folder with the belly schematics
- 'Head' folder with the head schematics
- '3Dmodels' and other folders regroups all the libraries, 3D models and foorprints for KiCad.
   
Software contains the gateware / software / SDK builder in expert mode to upload inside the robot.
It exists 2 softwares inside the robot. 
- The bootloader is the one called when you boot the robot. It contains a gateware and the pogobios.
- The user space is called by a command inside the bootloader. It contains your code and a gateware. <br>
/!\ If you plan to use the pogobot in the user space please use the sdk [here](https://github.com/nekonaute/pogobot-sdk).
  
Scripts corresponds to the differents scripts used around the robot.

## Shortcuts to

- [Robot Assembly for Users](./docs/RobotAssembly.md)
- [Software documentation for Users](./docs/SoftwareUserGuide.md)
- [Software documentation for Experts](./Software/readme.md)
- [Pogobot Remote Tool](./docs/IR-Remote.md)
- [Pogobot Charger Tool](./Hardware/readme.md#charger)
- [Pogobot Wall Tool](./Software/pogoWallApp/Readme.md)
- [Pogobject Tool](./Software/pogobject/readme.md)
- [Hardware documentation](./Hardware/readme.md)

# Pogobot Extended Universe

- [Pogobot Projet Entry Page](https://pogobot.github.io/)
- [SDK Repository](https://github.com/nekonaute/pogobot-sdk)
- [Pogobot2026 Tracking Software](https://github.com/keivan-amini/pogotrack)
- [Pogobot Simulator](https://github.com/Adacoma/pogosim)
- [Pogobot Software Addons](https://github.com/Adacoma/pogo-utils)


# Project History

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







