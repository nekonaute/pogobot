# <a name='ir-remote'></a>IR Remote
To control each robot, two ways are possible: by cable (debug mode) or by IR.
In order to control robots by IR, you need to create a IR remote.

## <a name='hardware'></a>Hardware
2 type of IR remote are available. A small one and one with a shower. 

The small one is combination of a pogobot head + usb-uart-progboard + 3D structure (available in the "3d-addons" folder) + 2 M3x10mm screws.

<img src="Images/ir_remote.jpg" alt="IR_ Remote" width="800"/>

The big one is combination of a pogobot head + remote + remote_Leds_strip + 3D structure

<img src="Images/ir_remote_shower.jpg" alt="IR_ Remote" width="800"/>

## <a name='software'></a>Software
In order to turn a head into a remote control, we need to put a special user code.

    ./pogosoc.py --target=pogobotv3 --cpu-variant=lite --build --remocon

Connect to the robot with the command

    ./litex_term.py --serial-boot --images images.json --safe /dev/ttyUSBX

Type the command

    serialboot

in order to upload the code. The robot reboot on the remote control code.


## <a name='usage'></a>Usage

Connect the remote from the folder where the usercode was compiled.

    make connect (TTY=/dev/ttyUSBX) (tty of the remote)

When a remote starts, it exposes the standard pogobot bios. You need to start the remote bios by typing : 

    run

A remote can't execute the command "run" and the command "serialboot" has a different meaning.
Once started different new commands are avaible :

- rc_start : start the user code on the robot (continious).
- rc_stop  : stop the user code and restart inside the pogobot bios (continious).
- rc_erase : erase the user code on the robot (continious).
- rc_send_bios_cmd <cmd> <args> : send the pogobios command by IR (once). 
- rc_send_bios_cmd_cont <delay> <cmd> <args> : send the pogobios command by IR every delay Ms (continious). 
- rc_send_user_msg <msg> : send the message by IR. It is not interpreted by the pogobios. The message type is 1. 
- rc_send_user_msg_cont <delay> <msg> : send the message by IR every delay Ms. It is not interpreted by the pogobios. The message type is 1 (continious). 
- serialboot : send the user code to the robots depending on the folder (once).  

To program a robot by IR, you need to clean the robots and reprogram its.<br>
Reboot the robot. The robots blink slowly green if a program is available. (if it is blue, no program available, no erase needed)<br>
"rc_erase" will erase the program and make its blink blue slowly. <br>
The command "serialboot" send to the robots a special command to switch to listen mode. Robots blink blue rapidily. <br>
When the programmation is done, the robots blink green slowly. <br>
If the robots blink orange, the programmation is partiel. Change the distance or angle of the remote and start again the "serialboot" command.
