# How to use the compiled binaries

## Using the ice40 dev board

If you have access to the ice40 dev board and the complementary pogobot board as in the Readme section " Upload an empty or faulty robot ",
you can use the script "program_remote.sh" or "program_robot.sh" available for the version you need.

## Using Litex_term.py

From the folder "pogobot/Software":

```
cp binary_installation/example_images.json .
```

Modify the path depending on the version needed and execute:

```
./litex_term.py --serial-boot --images example_images.json --safe /dev/ttyUSBX (X depending of the system)
```

reboot the robot, in order to be inside the "pogoboot >"

execute on the robot

```
serialboot
```

The robot downloads and restarts with the new code.

## In the case of the bootloader

In the folder "pogobot/Software":

```
./litex_term.py --serial-boot --kernel=binary_installation/install_APIv2.2.1/bootloader/bootloader_pogobotv3/bootloader.bin --kernel-adr=0x200000 --safe /dev/ttyUSBX
```

On the robot, execute:

```
serialboot
```
