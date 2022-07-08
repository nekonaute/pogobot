#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License.
# Please refer to file LICENCE for details.

"""
Created on Mon Dec  9 14:36:33 2019

@author: fabien
"""

from migen import *
import random
from ts4231 import PHYRX, PHYTX

class PADs():
    def __init__(self):
        self.rx = Signal()
        self.tx = Signal()
        self.env = Signal()

def data_generator(dut, datas):
    prng = random.Random(42)
    yield dut.TX.trig.eq(1)
    yield dut.serial.env.eq(1)
    for i, data in enumerate(datas):
        while prng.randrange(4):
            yield
        yield dut.TX.sink.data.eq(data)
        yield dut.TX.sink.valid.eq(1)
        yield
        while (yield dut.TX.sink.ready) == 0:
            yield
        yield dut.TX.sink.valid.eq(0)
        yield
    yield dut.TX.trig.eq(0)

    for i in range(512):
        yield

def data_checker(dut, datas):
    prng = random.Random(42)
    dut.errors = 0
    for i, reference in enumerate(datas):
        yield dut.RX.source.ready.eq(1)
        yield
        while (yield dut.RX.source.valid) == 0:
            yield
        data =  (yield dut.RX.source.data)
        if data != reference:
            dut.errors += 1
        yield dut.RX.source.ready.eq(0)
        while prng.randrange(4):
            yield


class DUT_WB_UART(Module):
    def __init__(self):
        self.serial = PADs()
        self.clock_domains.cd_sys = ClockDomain()

        self.submodules.TX = PHYTX(tx=self.serial.tx)
        #self.submodules.tx = ClockDomainsRenamer({"wb": "sys", "clk48": "clk48"})(TS4231PHYTX(serial))
        self.submodules.RX = PHYRX()
        self.comb += {
                self.RX.rx_raw.eq(self.TX.tx),
                self.RX.env_raw.eq(~self.serial.env), # Simulate inversion of env (normally high)
                self.RX.rx_enable.eq(1)
                }

def test_uart():
    prng = random.Random(17)
    dut = DUT_WB_UART()
    datas = [prng.randrange(2**20) for i in range(42)]
    datas += [0xCA, 0xFE, 0, 0xCA, 0XFE, 0xFA, 0xDE, 0x42, 0x21, 0x13, 0x37, 0]
    generators = [
        data_generator(dut, datas),
        data_checker(dut, datas)
    ]
    print("running sim...")
    #run_simulation(dut, generators)
    run_simulation(dut, generators, clocks={"sys": int(1000/21)}, vcd_name="ts4231.vcd") # clocks in nanoseconds
    #self.assertEqual(dut.errors, 0)


test_uart()
