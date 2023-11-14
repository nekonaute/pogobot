# How to use already compiled binaries

## Using the ice40 dev board
If your are access to the ice40 dev borad and the complementary pogobot board as in the Readme section " Upload a empty or faulty robot ",
you can use the script " program_remote.sh" or "program_robot.sh" available in the version you need.

## Using Litex_term.py

Inside the folder "pogobot/Software".

```
cp binary_installation/example_images.json .
```

modify the path depending on the version needed and execute

```
./litex_term.py --serial-boot --images example_images.json --safe /dev/ttyUSBX (X depending of the system)
```

reboot the robot, in order to be inside the "pogoboot >"

execute on the robot

```
serialboot
```

The robot download and restart on the new code