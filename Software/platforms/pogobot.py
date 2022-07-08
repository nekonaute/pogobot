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
    ("clk48", 0, Pins("44"), IOStandard("LVCMOS33")),

    ("user_led_n", 0, Pins("39"), IOStandard("LVCMOS33")),
    ("rgb_led", 0,
        Subsignal("r", Pins("39")),
        Subsignal("g", Pins("41")),
        Subsignal("b", Pins("40")),
        IOStandard("LVCMOS33"),
    ),

    ("serial", 0,
        Subsignal("tx", Pins("37")),
        Subsignal("rx", Pins("34")),
        IOStandard("LVCMOS33")
    ),
    ("usb", 0,
        # Oops, d_p and d_n swapped compared to original evt board. pullup and pulldown swapped too.
        Subsignal("d_p", Pins("37")),
        Subsignal("d_n", Pins("34")),
        Subsignal("pullup",   Pins("36")),
        #Subsignal("pulldown", Pins("35")),# is it usefull?
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
    ("isp_k210", 0,
        Subsignal("tx", Pins("26")), # RX on K210 and on PCB
        Subsignal("rx", Pins("25")), # TX on k210 and PCB
        Subsignal("boot_n", Pins("23")),
        Subsignal("reset_n", Pins("9")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 0, #HeadRight, BodyFront
        Subsignal("H", Pins("31")),
        Subsignal("L", Pins("32")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 1, #HeadFront, BodyLeft
        Subsignal("H", Pins("4")),
        Subsignal("L", Pins("6")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 2, #HeadLeft, BodyBack
        Subsignal("H", Pins("47")),
        Subsignal("L", Pins("48")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 3, #HeadBack, BodyRight
        Subsignal("H", Pins("46")),
        Subsignal("L", Pins("45")),
        IOStandard("LVCMOS33")
    ),
    ("TS4231", 0, #HeadRight, BodyFront
         Subsignal("D", Pins("27")),
         Subsignal("E", Pins("28")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 1, #HeadFront, BodyLeft
         Subsignal("D", Pins("10")),
         Subsignal("E", Pins("11")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 2, #HeadLeft, BodyBack
         Subsignal("D", Pins("2")),
         Subsignal("E", Pins("3")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 3, #HeadBack, BodyRight
         Subsignal("D", Pins("42")),
         Subsignal("E", Pins("43")),
         IOStandard("LVCMOS33")
    ),

    ("K210SPI", 0,
         Subsignal("miso", Pins("38")), # IC2IC1 - IO3 TP21
         Subsignal("mosi", Pins("21")), # IC2IC2 - IO21 TP19
         Subsignal("cs_n", Pins("20")), # IC2IC3 - IO20 TP16
         Subsignal("clk", Pins("13")), # IC2IC4 - IO31 TP13
         IOStandard("LVCMOS33")
    ),
    ("K210HSUART", 0, # High Speed UART
         Subsignal("tx", Pins("38")), # IC2IC1 - IO3 TP21
         Subsignal("rx", Pins("21")), # IC2IC2 - IO21 TP19
         IOStandard("LVCMOS33")
    ),
    ("IC2IC", 0,
         #Subsignal("tx", Pins("38")), # IC2IC1 - IO3 TP21
         #Subsignal("rx", Pins("21")), # IC2IC2 - IO21 TP19
         #Subsignal("cs_n", Pins("20")), # IC2IC3 - IO20 TP16
         #Subsignal("clk", Pins("13")), # IC2IC4 - IO31 TP13
         Subsignal("TP3", Pins("12")), # IC2IC5 - IO33 TP3
         IOStandard("LVCMOS33")
    ),
]

# Connectors ---------------------------------------------------------------------------------------

_connectors = [
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

