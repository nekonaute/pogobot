# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

# Pogobot design based on Fomu:
# - Crowd Supply campaign: https://www.crowdsupply.com/sutajio-kosagi/fomu
# - Design files: https://github.com/im-tomu/fomu-hardware/tree/evt/hardware/pcb

from litex.build.generic_platform import *
from litex.build.lattice import LatticePlatform
from litex.build.lattice.programmer import IceStormProgrammer

# IOs ----------------------------------------------------------------------------------------------

_io = [
    ("clk48", 0, Pins("35"), IOStandard("LVCMOS33")),

    ("user_led", 0, Pins("36"), IOStandard("LVCMOS33")),

    ("serial", 0,
        Subsignal("tx", Pins("26")),
        Subsignal("rx", Pins("27")),
        IOStandard("LVCMOS33")
    ),

    ("spiflash", 0,
        Subsignal("cs_n", Pins("16"), IOStandard("LVCMOS33")),
        Subsignal("clk",  Pins("15"), IOStandard("LVCMOS33")),
        Subsignal("miso", Pins("17"), IOStandard("LVCMOS33")),
        Subsignal("mosi", Pins("14"), IOStandard("LVCMOS33")),
        Subsignal("wp",   Pins("18"), IOStandard("LVCMOS33")),
        Subsignal("hold", Pins("19"), IOStandard("LVCMOS33")),
    ),

    ("spiflash4x", 0,
        Subsignal("cs_n", Pins("16"), IOStandard("LVCMOS33")),
        Subsignal("clk",  Pins("15"), IOStandard("LVCMOS33")),
        Subsignal("dq",   Pins("14 17 18 19"), IOStandard("LVCMOS33")),
    ),
    ("ir_tx", 0, #Front, 1=L, 2=H
        Subsignal("H", Pins("4")),
        Subsignal("L", Pins("48")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 1, #Right
        Subsignal("H", Pins("9")),
        Subsignal("L", Pins("12")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 2, #Back
        Subsignal("H", Pins("21")),
        Subsignal("L", Pins("20")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 3, #Left
        Subsignal("H", Pins("28")),
        Subsignal("L", Pins("47")),
        IOStandard("LVCMOS33")
    ),
    ("TS4231", 0, #Front
         Subsignal("D", Pins("2")),
         Subsignal("E", Pins("3")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 1, #Right
         Subsignal("D", Pins("10")),
         Subsignal("E", Pins("11")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 2, #Back
         Subsignal("D", Pins("23")),
         Subsignal("E", Pins("25")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 3, #Left
         Subsignal("D", Pins("31")),
         Subsignal("E", Pins("32")),
         IOStandard("LVCMOS33")
    ),

    ("adc_cs_n", 0, Pins("46"), IOStandard("LVCMOS33")),
    ("imu_cs_n", 0, Pins("6"), IOStandard("LVCMOS33")),
    ("ram_cs_n", 0, Pins("13"), IOStandard("LVCMOS33")),

    ("imu_int", 0, Pins("34"), IOStandard("LVCMOS33")),
]

# Connectors ---------------------------------------------------------------------------------------

_connectors = [
    # Left connector, J3, 30 pins (from top view)
    # Right connector, J5, 10 pins

    # Complete pinout :
    # 1 : RGB_LED_OUT (connected to output of head's RGB LED)
    # 2 : GPIO8
    # 3 : GND
    # 4 : CDONE
    # 3,5-7,,9 : GND
    # 8 : LED_Charge
    # 10 : iCE_RX
    # 11 : iCE_TX
    # 12,13,18,19 : +3.3V
    # 14,15 : +5V (charge)
    # 16,17 : +Batt
    # 20 : IMU_INT
    # 21 : unused
    # 22 : RGB_LED_IN (connected to iCE40)
    # 23 : GPIO7
    # 24 : GPIO6
    # 25 : GPIO5
    # 26 : GPIO4
    # 27 : GPIO3
    # 28 : GPIO2
    # 29 : GPIO1
    # 30 : GPIO0

    # usable pins connected to FPGA :
    # pin :   30 29 28 27 26 25 24 23  2 4 22
    ("GPIO", "44 43 42 41 40 39 38 37 45 7 36")
    # index :  0  1  2  3  4  5  6  7  8 9 10

    # J5 pinout, nothing really configurable here
    # 1 : FLASH_CS
    # 2 : SPI_MISO
    # 3 : SPI_MOSI
    # 4 : SPI_CLK
    # 5 : GND
    # 6 : RESET_n
    # 7 : ADC channel 4
    # 8 : ADC channel 5
    # 9 : ADC channel 6
    # 10 : ADC channel 7

]

# Platform -----------------------------------------------------------------------------------------

class Platform(LatticePlatform):
    default_clk_name   = "clk48"
    default_clk_period = 1e9/48e6

    def __init__(self):
        LatticePlatform.__init__(self, "ice40-up5k-sg48", _io, _connectors, toolchain="icestorm")

    def create_programmer(self):
        return IceStormProgrammer()

    def do_finalize(self, fragment):
        LatticePlatform.do_finalize(self, fragment)
        self.add_period_constraint(self.lookup_request("clk48", loose=True), 1e9/48e6)

