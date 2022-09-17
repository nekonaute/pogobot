#!/usr/bin/env python3

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

#
# This file is part of LiteX.
#
# Copyright (c) 2014 Yann Sionneau <ys@m-labs.hk>
# Copyright (c) 2014-2018 Florent Kermarrec <florent@enjoy-digital.fr>
# Copyright (c) 2013-2014 Robert Jordens <jordens@gmail.com>
# Copyright (c) 2015-2014 Sebastien Bourdeauducq <sb@m-labs.hk>
# Copyright (c) 2020 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: BSD-2-Clause


from migen import *
from migen.genlib.misc import timeline
from migen.fhdl.specials import Tristate

from litex.gen import *

from litex.soc.interconnect import wishbone
from litex.soc.interconnect.csr import *
from litex.soc.cores.spi import SPIMaster

# SpiFlash Quad/Dual/Single (memory-mapped) --------------------------------------------------------

_FAST_READ = 0x0b
_READ      = 0x03
_DIOFR     = 0xbb
_QIOFR     = 0xeb
_QIOPP     = 0x12

def _format_cmd(cmd, spi_width):
    """
    `cmd` is the read instruction. Since everything is transmitted on all
    dq lines (cmd, adr and data), extend/interleave cmd to full pads.dq
    width even if dq1-dq3 are don't care during the command phase:
    For example, for N25Q128, 0xeb is the quad i/o fast read, and
    extended to 4 bits (dq1,dq2,dq3 high) is: 0xfffefeff
    """
    c = 2**(8*spi_width)-1
    for b in range(8):
        if not (cmd>>b)%2:
            c &= ~(1<<(b*spi_width))
    return c

def accumulate_timeline_deltas(seq):
    t, tseq = 0, []
    for dt, a in seq:
        tseq.append((t, a))
        t += dt
    return tseq

class SpiFlashCommon(Module):
    def __init__(self, pads):
        if not hasattr(pads, "clk"):
            self.clk_primitive_needed = True
            self.clk_primitive_registered = False
            pads.clk = Signal()
        self.pads = pads

    def add_clk_primitive(self, device):
        if not hasattr(self, "clk_primitive_needed"):
            return
        # Xilinx 7-series
        if device[:3] == "xc7":
            self.specials += Instance("STARTUPE2",
                i_CLK       = 0,
                i_GSR       = 0,
                i_GTS       = 0,
                i_KEYCLEARB = 0,
                i_PACK      = 0,
                i_USRCCLKO  = self.pads.clk,
                i_USRCCLKTS = 0,
                i_USRDONEO  = 1,
                i_USRDONETS = 1
            )
        # Lattice ECP5
        elif device[:4] == "LFE5":
            self.specials += Instance("USRMCLK",
                i_USRMCLKI  = self.pads.clk,
                i_USRMCLKTS = 0
            )
        else:
            raise NotImplementedError
        self.clk_primitive_registered = True

    def do_finalize(self):
        if hasattr(self, "clk_primitive_needed"):
            assert self.clk_primitive_registered == True

class SpiFlashSingle(SpiFlashCommon, AutoCSR):
    def __init__(self, pads, dummy=15, div=2, with_bitbang=True, endianness="big", ram_pad=None, flashsize=0):
        """
        Simple memory-mapped SPI flash.
        Supports 1-bit reads. Only supports mode3 (cpol=1, cpha=1).
        """
        SpiFlashCommon.__init__(self, pads)
        self.bus = bus = wishbone.Interface()

        if with_bitbang:
            self.bitbang = CSRStorage(4, reset_less=True, fields=[
                CSRField("mosi", description="Output value for SPI MOSI pin."),
                CSRField("clk",  description="Output value for SPI CLK pin."),
                CSRField("cs_n", reset=1, description="Output value for SPI CSn pin."),
                CSRField("ram_cs_n", reset=1, description="Output value to SPI RAM CSn pin.")
            ], description="""Bitbang controls for SPI output.""")
            self.miso = CSRStatus(description="Incoming value of MISO pin.")
            self.bitbang_en = CSRStorage(description="Write a ``1`` here to disable memory-mapped mode and enable bitbang mode.")

        # # #

        if hasattr(pads, "wp"):
            self.comb += pads.wp.eq(1)

        if hasattr(pads, "hold"):
            self.comb += pads.hold.eq(1)
        if ram_pad is not None:
            with_ram = True
        else:
            with_ram = False

        if (flashsize == 0) and with_ram:
            raise ValueError("flash_size mush be specified")

        cs_n = Signal(reset=1)
        clk  = Signal()
        wbone_width = len(bus.dat_r)

        if dummy == 0:
            read_cmd = _READ
        else:
            read_cmd = _FAST_READ
        cmd_width = 8
        addr_width = 24

        sr = Signal(max(cmd_width, addr_width, wbone_width))
        if endianness == "big":
            self.comb += bus.dat_r.eq(sr)
        else:
            self.comb += bus.dat_r.eq(reverse_bytes(sr))

        #flashsize=0x100000 #(1MB)

        hw_read_logic = [
            pads.clk.eq(clk),
            pads.mosi.eq(sr[-1:])]
        if with_ram:
            hw_read_logic += [
                If((bus.adr>=flashsize),
                    pads.cs_n.eq(1),
                    ram_pad.ce_n.eq(cs_n)
                ).Else(
                    pads.cs_n.eq(cs_n),
                    ram_pad.ce_n.eq(1)
                )
            ]
        else:
            hw_read_logic += [ pads.cs_n.eq(cs_n) ]


        if with_bitbang:
            bitbang_logic = [
                pads.clk.eq(self.bitbang.storage[1]),
                pads.cs_n.eq(self.bitbang.storage[2]),
                If(self.bitbang.storage[1], # CPOL=0/CPHA=0 or CPOL=1/CPHA=1 only.
                    self.miso.status.eq(pads.miso)
                ),
                pads.mosi.eq(self.bitbang.storage[0]),
            ]
            if with_ram:
                bitbang_logic += [ram_pad.ce_n.eq(self.bitbang.storage[3])]

            self.comb += [
                If(self.bitbang_en.storage,
                    bitbang_logic
                ).Else(
                    hw_read_logic
                )
            ]

        else:
            self.comb += hw_read_logic

        if div < 2:
            raise ValueError("Unsupported value \'{}\' for div parameter for SpiFlash core".format(div))
        else:
            i = Signal(max=div)
            miso = Signal()
            self.sync += [
                If(i == div//2 - 1,
                    clk.eq(1),
                    miso.eq(pads.miso),
                ),
                If(i == div - 1,
                    i.eq(0),
                    clk.eq(0),
                    sr.eq(Cat(miso, sr[:-1]))
                ).Else(
                    i.eq(i + 1),
                ),
            ]

        # spi is byte-addressed, prefix by zeros
        z = Replicate(0, log2_int(wbone_width//8))

        seq = [
            (cmd_width*div,
                [cs_n.eq(0), sr[-cmd_width:].eq(read_cmd)]),
            (addr_width*div,
                [sr[-addr_width:].eq(Cat(z, bus.adr))]),
            ((dummy + wbone_width)*div,
                []),
            (1,
                [bus.ack.eq(1), cs_n.eq(1)]),
            (div, # tSHSL!
                [bus.ack.eq(0)]),
            (0,
                []),
        ]

        # accumulate timeline deltas
        t, tseq = 0, []
        for dt, a in seq:
            tseq.append((t, a))
            t += dt

        self.sync += timeline(bus.cyc & bus.stb & (i == div - 1), tseq)


def SpiFlash(pads, *args, **kwargs):
    if hasattr(pads, "mosi"):
       return SpiFlashSingle(pads, *args, **kwargs)

