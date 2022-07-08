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

    ("user_led", 0, Pins("34"), IOStandard("LVCMOS33")),

    ("serial", 0,
        Subsignal("tx", Pins("42")),
        Subsignal("rx", Pins("43")),
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
        Subsignal("tx", Pins("10")), # RX on K210 and on PCB
        Subsignal("rx", Pins("11")), # TX on k210 and PCB
        Subsignal("boot_n", Pins("2")),
        Subsignal("reset_n", Pins("36")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 0, #Front
        Subsignal("H", Pins("6")),
        Subsignal("L", Pins("9")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 1, #Right
        Subsignal("H", Pins("21")),
        Subsignal("L", Pins("20")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 2, #Back
        Subsignal("H", Pins("45")),
        Subsignal("L", Pins("46")),
        IOStandard("LVCMOS33")
    ),
    ("ir_tx", 3, #Left
        Subsignal("H", Pins("47")),
        Subsignal("L", Pins("48")),
        IOStandard("LVCMOS33")
    ),
    ("TS4231", 0, #Front
         Subsignal("D", Pins("13")),
         Subsignal("E", Pins("12")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 1, #Right
         Subsignal("D", Pins("27")),
         Subsignal("E", Pins("28")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 2, #Back
         Subsignal("D", Pins("38")),
         Subsignal("E", Pins("37")),
         IOStandard("LVCMOS33")
    ),
    ("TS4231", 3, #Left
         Subsignal("D", Pins("4")),
         Subsignal("E", Pins("3")),
         IOStandard("LVCMOS33")
    ),

    ("IC2IC", 0,
         Subsignal("IC2IC1", Pins("39")), # IC2IC1 - IO
         Subsignal("IC2IC2", Pins("40")), # IC2IC2 - IO
         Subsignal("IC2IC3", Pins("41")), # IC2IC3 - IO
         Subsignal("IC2IC4", Pins("35")), # IC2IC4 - IO
         IOStandard("LVCMOS33")
    ),

    ("Motors", 0, Pins("26"), IOStandard("LVCMOS33")),
    ("Motors", 1, Pins("31"), IOStandard("LVCMOS33")),
    ("Motors", 2, Pins("32"), IOStandard("LVCMOS33")),

    ("adc_cs_n", 0, Pins("25"), IOStandard("LVCMOS33")),
    ("imu_cs_n", 0, Pins("23"), IOStandard("LVCMOS33")),
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

