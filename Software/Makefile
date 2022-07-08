
TARGET?=pogobotv2_1

bootloader: build/$(TARGET)/gateware/$(TARGET).bin
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) --bootloader --build )

remocon: build/$(TARGET)/gateware/$(TARGET).bin
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) --bootloader --remocon --build )

gateware: build/$(TARGET)/gateware/$(TARGET).bin
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) --build )

software: build/$(TARGET)/software/pogobios/pogobios.bin
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) )


.PHONY: bootloader remocon gateware

