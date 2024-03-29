# How to use the compiled binaries

## Using the ice40 dev board

If you have access to the ice40 dev board and the complementary pogobot board as in the Readme section " Upload an empty or faulty robot ",
you can use the script "program_remote.sh" or "program_robot.sh" available for the version you need.

## Using Litex_term.py
### Installing the gateware and the pogobios

From the folder "pogobot/Software":

```
cp binary_installation/example_images.json .
```

Modify the path depending on the version needed and execute (replace XXX to match your configuration):

```
./litex_term.py --serial-boot --images example_images.json --safe /dev/ttyUSBXXX
```

Reboot the robot. It will then display the "pogoboot >" prompt

For the robot to download and restart with the new code, execute (on the robot):

```
serialboot
```

### Installing the remote gateware and pogobios

It is the same as the previous section but it uses the "example_images_remote.json" file instead.

/!\ The gateware of the remote and of a robot are different so the code of a robot can't run on a remote.

### In the case of the bootloader

In the folder "pogobot/Software" (replace XXX to match your configuration):

```
./litex_term.py --serial-boot --kernel=binary_installation/install_APIv2.2.1/bootloader/bootloader_pogobotv3/bootloader.bin --kernel-adr=0x200000 --safe /dev/ttyUSBXXX
```

On the robot, execute:

```
serialboot
```
