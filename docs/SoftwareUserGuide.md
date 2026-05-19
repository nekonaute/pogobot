# Software Guide for Users

A practical introduction to programming a Pogobot for the first time. This document has been generated using human written parts and assembled with ChatGPT.

---

# 1. Introduction and Setup

## How are pogobot programmed ?

A Pogobot has programs written in **C** and uploaded either:

* through a **USB cable** (single robot),
* or through a **remote infrared uploader** (“shower” / walls) for multiple robots.

Typical Pogobot programs:

* blink LEDs,
* move motors,
* communicate through infrared messages,
* react to sensors,
* coordinate with other robots.

---

## Installing the Environment

You need the Pogobot SDK and toolchain installed before compiling programs. See [SDK repository and installation instructions](https://github.com/nekonaute/pogobot-sdk/tree/main#pogobot). As a reference point once the SDK is installed, you can navigate to the `helloworld` example under `pogobot-sdk/examples/helloworld`.

---

## Typical Program Structure

A Pogobot project usually contains:

```text
my_program/
├── main.c
└── Makefile
```

With the Makefile being essentially the same in every program, and as such can be copied from examples. Most programs follow this structure:

```c
#include "pogobot.h"

int main(void)
{
    pogobot_init();

    while (1)
    {
        // read sensors

        // communicate

        // move

        // blink LEDs

        msleep(10);
    }
}
```

---

# 2. First Program and Core SDK Concepts

## Minimal Example

### main.c

```c
#include "pogobot.h"

int main(void)
{
    pogobot_init();

    printf("init ok\n");

    while (1)
    {
        // Blue LED
        pogobot_led_setColor(0, 0, 255);

        // Move left motor
        pogobot_motor_set(motorL, motorFull);
        pogobot_motor_set(motorR, motorStop);

        msleep(500);

        printf("HELLO WORLD !!!\n");

        // Red LED
        pogobot_led_setColor(255, 0, 0);

        // Move right motor
        pogobot_motor_set(motorL, motorStop);
        pogobot_motor_set(motorR, motorFull);

        msleep(500);
    }
}
```

This program:

* initializes the robot,
* alternates LED colors,
* alternates motor movement,
* prints text to the serial terminal.

---

## Initialization

Every program must start with:

```c
pogobot_init();
```

Without initialization, most hardware features will not work.

---

## LEDs

Set the head LED color:

```c
pogobot_led_setColor(r, g, b);
```

Each value ranges from:

```text
0 → 255
```

Example:

```c
pogobot_led_setColor(255, 0, 0);
```

Red LED.

---

## Timing

Most programs use:

```c
msleep(milliseconds);
```

Example:

```c
msleep(500);
```

Waits 500 ms.

---

## Motors

Pogobot has 3 vibration motors:

```text
motorR
motorL
motorB
```

Motor power levels:

```text
motorStop          = 0
motorQuarter       = 256
motorHalf          ≈ 512
motorThreeQuarter  ≈ 716
motorFull          = 1023
```

Example:

```c
pogobot_motor_power_set(motorL, motorFull);
pogobot_motor_power_set(motorR, motorStop);
```

---

## Debug Printing

Use standard C printing:

```c
printf("Hello\n");
```

Messages appear in the serial terminal and are extremely useful for debugging.

---

## Sensors

### Ambient Light Sensors

```c
int value = pogobot_photosensors_read(0);
```

Sensors:

```text
0 = back
1 = front-left
2 = front-right
```

---

### Battery Voltage

```c
int mv = pogobot_battery_voltage_read();
```

Returns millivolts.

---

### IMU

```c
float acc[3];
float gyro[3];

pogobot_imu_read(acc, gyro);
```

Axes:

```text
0 = X
1 = Y
2 = Z
```

---

## Timers

### Stopwatch Example

```c
time_reference_t timer;

pogobot_stopwatch_reset(&timer);

// some computation

int32_t elapsed =
    pogobot_stopwatch_get_elapsed_microseconds(&timer);

printf("%ld\n", elapsed);
```

---

### Periodic Timer Example

```c
time_reference_t timer;

pogobot_timer_init(&timer, 1000000);

while (1)
{
    if (pogobot_timer_has_expired(&timer))
    {
        printf("1 second elapsed\n");

        pogobot_timer_offset_origin_microseconds(
            &timer,
            1000000
        );
    }
}
```

---

# 3. Infrared Communication and Swarm Programming

Pogobots communicate using infrared.

Typical workflow:

1. send a message,
2. periodically update IR system,
3. check if messages arrived,
4. read messages.

---

## Sending a Message

```c
char msg[] = "hello";

pogobot_infrared_sendShortMessage_omni(
    (uint8_t*)msg,
    sizeof(msg)
);
```

This broadcasts in all directions.

---

## Receiving Messages

```c
pogobot_infrared_update();

if (pogobot_infrared_message_available())
{
    message_t msg;

    pogobot_infrared_recover_next_message(&msg);

    printf("Message received\n");
}
```

---

## Example: Distributed Genetic Algorithm

The following program demonstrates:

* infrared swarm communication,
* distributed decision-making,
* mutation and selection,
* LEDs,
* motors,
* random genome generation.

Each robot:

1. generates a random binary genome,
2. computes a fitness score,
3. broadcasts its genome,
4. copies stronger genomes from neighbors,
5. occasionally mutates,
6. gradually converges toward an optimal genome.

This example comes from the distributed genetic algorithm demo in the SDK, `examples/demo_d2_9BIS`.

### Key Ideas

Genome representation:

```c
typedef struct my_data_t
{
    uint16_t genome;
    uint16_t fitness;
} my_data_t;
```

Broadcasting data:

```c
pogobot_infrared_sendLongMessage_omniGen(
    (uint8_t *)( &my_data ),
    sizeof( my_data )
);
```

Receiving and comparing genomes:

```c
if ( recept->fitness > my_data.fitness )
{
    my_data.genome = recept->genome;
}
```

Mutation:

```c
my_data->genome ^= 1UL << n;
```

This type of program is useful for experimenting with:

* swarm intelligence,
* collective optimization,
* decentralized coordination,
* emergent behavior.

---

# 4. Compiling, Uploading, and Running Programs

## Compiling

Inside your project folder:

```bash
make
```

If compilation succeeds, you should obtain:

```text
build/firmware.bin
build/firmware.elf
```

---

## Hardware Setup

For USB programming:

1. connect the robot to the programming board using the ribbon cable,
2. connect the programming board to your computer with USB.

When correctly powered, LEDs usually blink several colors during startup.

---

## Uploading Through USB

Connect to the robot:

```bash
make connect TTY=/dev/ttyUSB0
```

Replace:

```text
/dev/ttyUSB0
```

with the correct serial device on your machine.

---

## Terminal Workflow

After running `make connect`:

1. press ENTER,
2. wait for the prompt,
3. use terminal commands directly.

Upload firmware:

```text
serialboot
```

Run the uploaded program:

```text
run
```

Exit terminal:

```text
Ctrl+C Ctrl+C
```

---

## Uploading to Multiple Robots (Infrared Remote)

Connect the remote uploader:

```bash
make connect TTY=/dev/ttyUSB0
```

Then:

1. press ENTER,
2. type:

```text
run
```

3. power on robots,
4. erase firmware:

```text
rc_erase
```

5. flash robots:

```text
rc_flash_robot
```

6. start all robots:

```text
rc_start
```

Robots typically turn green when upload succeeds.

---

## Important Warning

Never use:

```text
serialboot
```

when connected to the infrared remote uploader.

Doing so overwrites the remote firmware itself.

---

# 5. Workflow, Troubleshooting, and Safety

## Recommended Development Workflow

A common workflow is:

1. modify `main.c`,
2. compile:

```bash
make
```

3. connect:

```bash
make connect
```

4. upload:

```text
serialboot
```

5. run:

```text
run
```

6. observe LEDs, movement, and debug prints,
7. repeat.

---

## Troubleshooting

### Robot Does Not Respond

Try:

* unplug/replug USB,
* restart terminal,
* power cycle robot,
* verify serial device path.

---

### Infrared Stops Working

Firmware 2.6 has a known IR bug.

Avoid resetting robots while they receive IR messages.

If IR breaks:

1. move robot away from others,
2. reset several times,
3. power cycle,
4. reflash firmware if needed.

---

### Program Too Large

Firmware 2.6 may fail when:

* `.data` exceeds about 41960 bytes,
* and the robot is started through `rc_start`.

Check size with:

```bash
riscv64-unknown-elf-size --format=SysV build/bin/firmware.elf
```

---

## Updating Robot Firmware

Firmware update instructions:

[Firmware update instructions](https://github.com/nekonaute/pogobot/blob/main/Software/Readme.md#upload-a-empty-or-faulty-robot)

Typical commands:

```bash
./program_robot.sh
```

or

```bash
./program_remote.sh
```

depending on the device.

---

## Safety Rules

* Power down robots before charging.
* Never leave robots charging unattended overnight.
* Do not leave robots on the arena after experiments.
* Fully charging takes about 2 hours.
* Avoid resetting robots during IR communication.

---

## Recommended Next Steps

After completing your first program:

1. explore the `examples/` folder,

2. experiment with:

   * swarm communication,
   * obstacle avoidance,
   * synchronization,
   * light-following behaviors,

3. create reusable helper functions,

4. test multi-robot behaviors with the infrared uploader.
