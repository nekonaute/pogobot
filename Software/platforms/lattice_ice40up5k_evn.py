# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

from litex.build.generic_platform import *
from litex.build.lattice import LatticePlatform
from litex.build.lattice.programmer import IceStormProgrammer


_io = [
    # Clk
    ("clk12", 0, Pins("35"), IOStandard("LVCMOS33")),

    # LEDs
    ("user_led_n", 0, Pins("39"), IOStandard("LVCMOS33")),
    ("rgb_led", 0,
        Subsignal("r", Pins("41")),
        Subsignal("g", Pins("40")),
        Subsignal("b", Pins("39")),
        IOStandard("LVCMOS33")
    ),

    # Buttons
    ("user_sw", 0, Pins("23"), IOStandard("LVCMOS33")),
    ("user_sw", 1, Pins("25"), IOStandard("LVCMOS33")),
    ("user_sw", 2, Pins("34"), IOStandard("LVCMOS33")),
    ("user_sw", 3, Pins("43"), IOStandard("LVCMOS33"))
]

spiflash = [
    # Only usable in PROG FLASH mode and J7 attached (see PCB silkscreen).
    ("spiflash", 0,
        Subsignal("cs_n", Pins("16"), IOStandard("LVCMOS33")),
        Subsignal("clk", Pins("15"), IOStandard("LVCMOS33")),
        Subsignal("mosi", Pins("14"), IOStandard("LVCMOS33")),
        Subsignal("miso", Pins("17"), IOStandard("LVCMOS33")),
    ),
]

serial = [
    ("serial", 0,
        Subsignal("tx", Pins("J3:0")),
        Subsignal("rx", Pins("J3:1")),
        IOStandard("LVCMOS33")
    ),
    ("serial", 1,
        Subsignal("tx", Pins("J3:2")),
        Subsignal("rx", Pins("J3:3")),
        IOStandard("LVCMOS33")
    )
]

IR = [
    ("ir_tx", 0,
        Subsignal("H", Pins("J3:6")),
        Subsignal("L", Pins("J3:7")),
        IOStandard("LVCMOS33")
    ),

    ("TS4231", 0,
        Subsignal("D", Pins("J3:4")),
        Subsignal("E", Pins("J3:5")),
        IOStandard("LVCMOS33")
    ),
    ]

_connectors = [
    # Many pins on the AARDVARK, PMOD, J52/HEADER A, and J2/HEADER B connectors
    # are multiplexed with other I/O or connector pins. For completeness, all
    # pins are exposed here except Vdd, NC, and GND. Pin order is as specified
    # on the schematic (except for PMOD, which uses Digilent's numbering).

    # AARDVARK connector omitted- although sysCONFIG pins are exposed on this
    # header (which can be used as GPIO), it is meant for flashing using an
    # external programmer rather than as an I/O port.

    # PMOD connector shares pins with sysCONFIG- make sure to remove jumper
    # J7 if using the PMOD. TODO: Perhaps it would be better to split into two
    # single 6-pin PMODs.
    #
    # PMOD pinout (using ICE40 pin names):
    # 1, 2, 3, 4- SPI_SS, SPI_SI, SPI_SO, SPI_SCK
    # 5, 6, 7, 8- Free
    ("PMOD", "16   17   14  15  27  26  32  31"),
    #Silk:    SS MOSI MISO SCK 38B 39A 43A 42B

    # J52 exposes LEDs and sysCONFIG pins (yet again). Make sure to remove
    # jumper J7 if using the PMOD. Pin order is as follows (right to left):
    # 12 10 8 6 4 2
    # 11  9 7 5 3 1
    #
    # J52's pinout (using ICE40 pin names for SPI flash):
    # 1, 2- Vdd
    # 3, 4- rgb_led.b, SPI_SI
    # 5, 6- rgb_led.g, SPI_SO
    # 7, 8- GND, SPI_SCK
    # 9, 10- rgb_led.r, SPI_SS
    # 11, 12- GND
    #         3  4  5  6  8  9 10
    ("J52", "39 17 40 14 15 41 16"),

    # Pin order of J2, and J3 are as follows (left to right/top to bottom):
    # 2 4 6 8 10 12 14 16 18 20
    # 1 3 5 7 9  11 13 15 17 19
    #
    # J2's pinout is shared by many things. Entire pinout listed follows:
    # 1, 2- Vdd
    # 3, 4- user_sw0, NC
    # 5, 6- user_sw1, NC
    # 7, 8- PMOD D5, Free
    # 9, 10- PMOD D4, Free
    # 11, 12- PMOD D6, Free
    # 13, 14- PMOD D7, Free
    # 15, 16- Free, 12.00 clock
    # 17, 18- user_sw2, GND
    # 19, 20- user_sw3, GND
    #        3   5   7   8   9  10  11  12  13  14     15      16  17  19
    ("J2", "23  25  26  36  27  42  32  38  31  28     37      35  34  43"),
    # Silk:37A 36B 39A 48B 38B 51A 43A 50B 42B 41A 45A_G1 ICE_CLK 44B 49A 
    # index: 0   1   2   3   4  5    6   7   8   9     10      11  12  13

    # Bank2: 4 3 48 45 47 44 46 2
    # Bank1: 12 21 13 20 19 18 11 10 9 6
    # J3's pinout is all Free, except 1 (Vdd) and 19 (GND).
    #        2  3   4  5   6  7      8  9  10 11  12    13  14 15  16 17  18  20
    ("J3", "12  4  21  3  13 48     20 45  19 47  18    44  11 46  10  2   9   6"),
    # Silk:22A 8A 23B 9B 24A 4A 25B_G3 5B 29B 2A 31B 3B_G6 20A 0A 18A 6A 16A 13B
    # index: 0  1   2  3   4  5      6  7   8  9  10    11  12 13  14 15  16  17
]


class Platform(LatticePlatform):
    default_clk_name = "clk12"
    default_clk_period = 1e9/12e6

    def __init__(self):
        LatticePlatform.__init__(self, "ice40-up5k-sg48", _io, _connectors,
                                 toolchain="icestorm")
        self.add_extension(serial)
        self.add_extension(spiflash)
        self.add_extension(IR)

    def create_programmer(self):
        return IceStormProgrammer()

    def do_finalize(self, fragment):
        LatticePlatform.do_finalize(self, fragment)
        self.add_period_constraint(self.lookup_request("clk12", loose=True), 1e9/12e6)

