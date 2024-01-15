# Hardware

## Hardware schematic
Here you can find the global view of the hardware.

<img src="../Images/pogobot_diagram.jpg" alt="diagram" width="800"/>

## What s needed for 1 robot

In order to complete a robot, you will need : 
- a belly and a head
- a 3v CR2 rechargeable Batery (Li-FePO4)
- 3 motors
- the different 3d printed parts

## Motor information
All the models are designed for the motors available here:
https://www.aliexpress.com/item/32918768766.html

<img src="../Images/motors.jpg" alt="motor" width="400"/>

## 3d printed parts
All the parts are available inside the folder 'Hardware/3d-addons'.

### Printing info
The different models have been printed in PLA with an Ultimaker2+ with a layer height of 0.2mm without support or brim.

To assemble a robot, you need to print a skirt, a capsule and a hat for the version of your belly. <br>
After printed the capsule, you have to set free the tabs by gently cut under.

<img src="../Images/cut_tab.jpg" alt="cut_tab" width="400"/>

HINT: The hat has to be returned to be printed without support.

### Assembly
- First, place the motors inside the 3 holes.
- Then, place the robot inside the capsule.
- Finally, place the robot+capsule inside the skirt.

<img src="../Images/assembly.jpg" alt="assembly" width="400"/>

## Programming the robot (hardware part)
In order to program the robot, you need a USB to UART device connected between your computor and the FFC/FPC connector on the head. 
The folder 'Hardware/usb-uart-progboard' contains the Kicad files to produce a small board to program the robot. 
You also need :
- UMFT234XF board (https://www.mouser.fr/ProductDetail/FTDI/UMFT234XF?qs=NVftvxkm2thPJLzIkoAhbA%3D%3D)
- a FFC/FPC connector (8 pins, 0.5 mm, SMD/SMT, Right Angle, Dual Contact) like https://www.mouser.fr/ProductDetail/571-2328702-8
- a FFC/FPC cable (8 pins, 0.5 mm) like https://www.mouser.fr/ProductDetail/538-15166-0087
- a USB micro => USB cable