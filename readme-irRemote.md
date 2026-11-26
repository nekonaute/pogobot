# IR Remote
To control each robot, two ways are possible: by cable (debug mode) or by IR.
In order to control robots by IR, you need to create a IR remote.

## Hardware
Please refer to the section "IR Remote" [here](/Hardware/readme.md). 

## Software
In order to turn a head into a remote control, we need to put a special user code.

    ./pogosoc.py --target=pogobotv3 --cpu-variant=lite --build --remocon

Connect to the robot with the command

    ./litex_term.py --serial-boot --images images.json --safe /dev/ttyUSBX

Type the command

    serialboot

in order to upload the code. The robot reboot on the remote control code.


## Usage

Connect the remote from the folder where the usercode was compiled.

    make connect TTY=/dev/ttyUSBX

When a remote starts, it exposes the standard pogobot bios. You need to start the remote bios by typing : 

    run

A remote can't execute the command "run" and the command "serialboot" has a different meaning.
Once started different new commands are avaible :

- rc_start : start the user code on the robot (continious).
- `rc_load_run <name>`: repeatedly ask robots to install and start that named PFFS controller; red LED on failure.
- rc_stop  : stop the user code and restart inside the pogobot bios (continious).
- rc_erase : erase the user code on the robot (continious).
- rc_send_bios_cmd <cmd> <args> : send the pogobios command by IR (once). 
- rc_send_bios_cmd_cont <delay> <cmd> <args> : send the pogobios command by IR every delay Ms (continious). 
- rc_send_user_msg <msg> : send the message by IR. It is not interpreted by the pogobios. The message type is 1. 
- rc_send_user_msg_cont <delay> <msg> : send the message by IR every delay Ms. It is not interpreted by the pogobios. The message type is 1 (continious). 
- rc_flash_robot ( old serialboot) : send the user code to the robots depending on the folder (once).  

To program a robot by IR, you need to clean the robots and reprogram its.<br>
Reboot the robot. The robots blink slowly green if a program is available. (if it is blue, no program available, no erase needed)<br>
"rc_erase" will erase the program and make its blink blue slowly. <br>
The command "rc_flash_robot" send to the robots a special command to switch to listen mode. Robots blink blue rapidily. <br>
When the programmation is done, the robots blink green slowly. <br>
If the robots blink orange, the programmation is partiel. Change the distance or angle of the remote and start again the "rc_flash_robot" command.

### Start a controller stored in PFFS

With the updated BIOS on both remote and receiving robots, use:

```text
rc_load_run controller_a
```

Like `rc_start`, this sends a command every 200 ms until a key is pressed.
The file must already be stored on each receiving robot; the remote does not
need a copy. Robots must be waiting at `Pogoboot>` or `Pogobot>`. Each resolves
the exact name, verifies and installs the raw controller binary, then reboots
to the user image. Numeric labels are treated as names, not file IDs.
Names must be 1..32 printable ASCII bytes without spaces.

A missing file, incompatible controller, CRC failure or installation failure
leaves the robot in its BIOS with a red LED/idle blink. It never falls back to
starting the previously installed program. Another BIOS command can restore
the ordinary marker-based LED status. Active UART transfers or pending IR file
reception block this request; finish or abort those first. There are no robot
acknowledgements. A robot can also execute `load_run <name>` locally over UART;
the existing `load <id|name>` command still installs without starting and is
UART-only. This feature still needs hardware validation.

### Reliable IR firmware upload

Install matching current Pogobios builds on the remote and robots, and update
the SDK tools used by the example Makefile so they include the current
`litex_term.py` and `ir_upload_v2.py`. For a firmware image, use the normal
command in the example directory:

    make connect TTY=/dev/ttyUSBX

After starting the remote with `run`, enter `rc_flash_robot` at its prompt.
The updated remote announces versioned IR support before requesting the image,
and the updated terminal selects that mode automatically for the single firmware
image in the example Makefile. `--ir-v2` remains an optional manual override;
it is not needed for this command. IR programming requires versioned frames;
legacy LOAD/JUMP/ABORT broadcasts are rejected. Direct cable uploads use the
existing serial protocol. Use a two-entry `images.json` over a cable, or upload
each image separately for versioned IR transfers.

The current version accepts one image per transfer in the gateware slot
(`0x240000`) or firmware slot (`0x260000`), up to 128 KiB. The PC sends a versioned
START, numbered 64-byte DATA chunks, four Reed–Solomon parity frames per group
of 16 data chunks, and two END messages per pass. Four groups are interleaved
to spread short bursts of lost frames. Up to four missing frames among a
group's 20 data and parity frames can be recovered. The normal command uses
one coded pass. For additional full passes, use for example:

    POGOBOT_IR_COPIES=2 make connect TTY=/dev/ttyUSBX

Valid copy counts are 1–5. `POGOBOT_IR_FEC=0 make connect TTY=/dev/ttyUSBX`
disables parity for comparison and defaults to three uncoded passes. Full
passes use the same transfer ID and retain accepted chunks. The robot erases
the destination at the first START, skips duplicate chunks, reads every write
back, and checks a CRC-32 over the exact image before writing `FlashIsOK`.
A robot that still misses chunks keeps the image invalid. A new transfer ID
restarts and erases the image; repeated START with the same ID and metadata
preserves progress in RAM until reboot.

The remote still pauses 200 ms after each data or parity frame and three
seconds after START for flash erase. At 60 KiB, 960 data plus 240 parity frames
make those pauses about four minutes for one coded pass, before other overhead.
The terminal prints time and remote acknowledgement per pass. Robot logs
report recovered chunks, decoder time, flash erase/write time, and drop
counters; use hardware trials to determine whether pacing can safely change.

Serial acknowledgements confirm processing by the remote, not successful
reception or completion by each robot. Check each robot's status before running
the new image.
