#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

from migen import *
import random
from neopixel import NeoPixelEngine

def bench(dut):
    for i in range(100):
        yield
    for data in (0x02010101, 0x03030202, 0x04040403, 0x00050505):
        yield from dut.neo._data.write(data)

    for i in range(10000):
        yield

class DUT_WB_NEO(Module):
    def __init__(self):
        self.output = Signal()
        self.clock_domains.cd_sys = ClockDomain()

        self.submodules.neo = NeoPixelEngine()
        self.comb += {
                self.output.eq(self.neo.DataPin),
                }

def test_neo():
    prng = random.Random(17)
    dut = DUT_WB_NEO()
    print("running sim...")
    run_simulation(dut, bench(dut), clocks={"sys": int(1000/21)}, vcd_name="neopixel.vcd") # clocks in nanoseconds

test_neo()

