
TARGET?=pogobotv2_1

bootloader:
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) --bootloader --build )

remocon:
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) --bootloader --remocon --build )

gateware:
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) --build )

software: 
	@( ./pogosoc.py --cpu-variant=lite --target=$(TARGET) )

all: gateware 

.PHONY: bootloader remocon gateware

