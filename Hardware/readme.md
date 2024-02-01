# Hardware
Here you can find the description and information for each part developped.

<!-- vscode-markdown-toc -->
- [Hardware](#hardware)
  - [Folder description](#folder-description)
  - [Pogobot robot](#pogobot-robot)
    - [Hardware schematic](#hardware-schematic)
    - [What's needed for 1 robot](#whats-needed-for-1-robot)
    - [Motor information](#motor-information)
    - [3d printed parts](#3d-printed-parts)
      - [Printing info](#printing-info)
      - [Assembly](#assembly)
  - [USB UART progboard](#usb-uart-progboard)
  - [Charger](#charger)
  - [IR remote](#ir-remote)
  - [IR Wall](#ir-wall)
  - [Pogobject](#pogobject)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->

## Folder description
In this folder, you can find 3d models and kicad files.

The folder :
- "3d-addons" contains all the STL files to build the different object.
- "3Dmodels" contains the 3D models for Kicad
- 'Belly" contains the kicad files for the belly
- "Charger" contains the kicad files for the charger
- "Footprints.pretty" is the Kicad library for custom footprints
- "Head" contains the kicad files for the head
- "pogo_object" contains the kicad files for the pogobject
- "remote" contains the kicad files for the board which host the head of a remote
- "remote_leds_strip" contains the kicad files for the "shower" part of a remote
- "remote_leds_wall_ctrl" contains the kicad files for the amplification signal for a led strip
- "remote_leds_wall_strip" contains the kicad files for the strip leds of a wall remote
- "Symbols" contains the custom symbols for kicad
- "usb-uart-progboard" contains the kicad files for the programmation board by cable


## <a name='Pogobotrobot'></a>Pogobot robot

### <a name='Hardwareschematic'></a>Hardware schematic
Here you can find the global view of the hardware.

<img src="../Images/pogobot_diagram.jpg" alt="diagram" width="800"/>

### <a name='Whatsneededfor1robot'></a>What's needed for 1 robot

In order to complete a robot, you will need : 
- a belly and a head
- a 3v CR2 rechargeable Batery (Li-FePO4)
- 3 motors
- the different 3d printed parts

### <a name='Motorinformation'></a>Motor information
All the models are designed for the motors available here:
<!-- https://www.aliexpress.com/item/32918768766.html old ref -->
https://www.ebay.com/itm/115374108208

<img src="../Images/motors.jpg" alt="motor" width="400"/>

### <a name='dprintedparts'></a>3d printed parts
All the parts are available inside the folder 'Hardware/3d-addons'.

#### <a name='Printinginfo'></a>Printing info
The different models have been printed in PLA with an Ultimaker2+ with a layer height of 0.2mm without support or brim.

To assemble a robot, you need to print a skirt, a capsule and a hat for the version of your belly. <br>
After printed the capsule, you have to set free the tabs by gently cut under.

<img src="../Images/cut_tab.jpg" alt="cut_tab" width="400"/>

HINT: The hat has to be returned to be printed without support.

#### <a name='Assembly'></a>Assembly
- First, place the motors inside the 3 holes.
- Then, place the robot inside the capsule.
- Finally, place the robot+capsule inside the skirt.

<img src="../Images/assembly.jpg" alt="assembly" width="400"/>

## <a name='USBUARTprogboard'></a>USB UART progboard
In order to program the robot by cable, you need a USB to UART device connected between your computor and the FFC/FPC connector on the head. 
The folder 'Hardware/usb-uart-progboard' contains the Kicad files to produce a small board to program the robot. 
You also need :
- UMFT234XF board (https://www.mouser.fr/ProductDetail/FTDI/UMFT234XF?qs=NVftvxkm2thPJLzIkoAhbA%3D%3D)
- a FFC/FPC connector (8 pins, 0.5 mm, SMD/SMT, Right Angle, Dual Contact) like https://www.mouser.fr/ProductDetail/571-2328702-8
- a FFC/FPC cable (8 pins, 0.5 mm) like https://www.mouser.fr/ProductDetail/538-15166-0087
- a USB micro => USB cable

## <a name='Charger'></a>Charger
There is two ways to recharge a pogobot:
- by the programming cable
- by the plus and minus pads on the head

In order to charge multiple robot at once, we develop a charger using pogopins that connect the plus and minus on the head of the robot by flipping the robot.

<img src="../Images/charger.jpg" alt="pogobot charger" width="800"/>

The PCB cad files are available in the folder.
The pogopin used are these : https://www.digikey.fr/en/products/detail/preci-dip/90224-AS/5451911
We also used one power jack 5.1 embase connected to the power outlet. The second one is used, if you want to daisy chain multiple PCB (be careful, the total amps used).
:warning: The power plug has to expose the +5v on the inner contact and Ground on the outer contact. Please verify with a voltmeter before pluggin a robot.

We developed some small feet to mount with M3 screws on the PCB and some small cones in order to help the contacts for each robot.
Both 3d designs are available in the folder "3d-addons" (pogocharger*).

## <a name='IRremote'></a>IR remote
2 types of IR remote are possible. A small one and one with a shower. 

The small one is combination of a pogobot head + usb-uart-progboard + 3D structure (pogoremote_down.stl + pogoremote_up.stl) + 2 M3x10mm screws.

<img src="../Images/ir_remote.jpg" alt="IR_ Remote" width="800"/>

The big one is combination of a pogobot head + remote + remote_Leds_strip + 3D structure

The leds are TSAL6200.

<img src="../Images/ir_remote_shower.jpg" alt="IR_ Remote" width="800"/>

This version can use the USB alimentation or an external one (up to 12v) to extend the range of the remote. The jumper near the power jack determine the origin of the tension for the Leds.


## <a name='IRWall'></a>IR Wall 

The IR wall is a modified remote that can connect a long strip of Leds.
We didn't manage to find the correct Led strip for our application so we designed a strip led (remote_leds_wall_strip) that was produiced on flexible pcb. 

<img src="../Images/remote_wall.jpg" alt="IR_ Remote" width="800"/>

To produice a IR wall, you need a pogobot head + remote + remote_leds_wall_ctrl + remote_leds_wall_strip. 

The leds are VSM3940x01.

## <a name='Pogobject'></a>Pogobject

The pogobject is composed of a dfrobot firebeatle + pogobot head + pogo_object PCB. Through WIFI, it can be possible to send command to the pogobot head and so to change the message send by the object.

<img src="../Images/pogobject.jpg" alt="IR_ Remote" width="800"/>

The pogobject is a large PCB that contains all the electronics. It is placed inside a 3d printed structure. This structure is placed on 4 caster with metal ball (pololu item 953) in order to be pushed by the robot. To make it fixed a aluminum weight is used to held it in place. 

The leds are TSAL6200.