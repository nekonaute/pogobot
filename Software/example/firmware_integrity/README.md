# Firmware integrity upload example

This example checks the complete installed `firmware.bin` in SPI flash,
including its executable code, initialized data, a generated payload, and
the payload-length field. The final four bytes hold the expected CRC-32.
The robot prints `FIRMWARE INTEGRITY PASS` or `FAIL` and lights its LED green
or red. A damaged program may fail to boot before it can report a result.

Build the v3 SDK first, then:

```sh
cd Software/example/firmware_integrity
make
make connect TTY=/dev/ttyUSB0
```

The default 32 KiB payload produces a 56.4 KiB image with the current v3 SDK.
Set `PAYLOAD_BYTES` to adjust the upload size; `make` reseals the image
without recompiling the program. It prints the exact final byte count, so
adjust the payload to keep the image in the 50–60 KiB range for your SDK
build. The payload is deterministic and stored only in flash; it does not
consume robot SRAM. The upload uses the ordinary `make connect` path.
