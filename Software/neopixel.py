#!/usr/bin/env python3

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

from migen import *
from litex.soc.interconnect.csr import *
from litex.soc.interconnect.csr_eventmanager import *
from litex.soc.interconnect import wishbone, stream
from litex.soc.integration.doc import AutoDoc,ModuleDoc

from math import ceil

class NeoPixelEngine(Module, AutoCSR, AutoDoc, ModuleDoc):
    """
    Simple NeoPixelEngine class provides the protocol logic to drive one or n NeoPixel LED.

    Usage:
######

#.  Fill NeoPixelEngine's Data register of GRB values (Green/Red/Blue).

#.  Trigger the logic by setting ``enable`` bit to true (1).

#######

    :DataPin:      NeoPixel 'Din' pin output (wire to actual output pin ...)
    """

    def __init__(self, n_leds=5, dw=24):

        # Inputs
        self._data = CSR(dw)

        # Local data
        GRBdata = Signal(dw, reset_less=True)                # Current 24-bit data to send
        PulseLen = Signal(11, reset_less=True)               # Current pulse length
        BitCount = Signal(max=dw, reset_less=True)
        LedCount = Signal(max=n_leds, reset_less=True)
        fifo_size = ceil(n_leds*24/dw)
        self.fifo = fifo = stream.SyncFIFO([("data", dw)], fifo_size) #, buffered=True)
        self.submodules += fifo

        # Output
        self.DataPin = Signal(1)            # To be wired to data pin ...

        self.enable = CSRStorage(1, reset_less=True,
            fields=[CSRField("enable", size=1, pulse=True, description="*Field*: bit", values=[
                    ("1", "ENABLED", "``NeoPixel`` protocol active for one cycle")]),
            ])
        self._fifo  = CSRStatus()
        #1, fields=[
        #            CSRField("fifo_empty", description= "Status of FIFO"),
        #        ])


        # # #

        self.comb += [
                fifo.sink.valid.eq(self._data.re),
                fifo.sink.data.eq(self._data.r),
                self._fifo.status.eq(~self.fifo.source.valid),   # source.valid.eq(fifo.readable), 
                ]

        # Inolux: 200ns<T0H<400ns, T0L>=800ns, 620ns<T1H<1us, T1L>=200ns, reset >80us
        # Mean values : T0H=336ns=7 cycles@21MHz; T0L=816ns=17cycles, T1H=672ns=14 cycles, T1L=240ns=5 cycles, reset > 1666 cycles
        T0H=4   # + LOADCOUNTER cycle
        T0L=17
        T1H=13
        T1L=4

        fsm = FSM(reset_state="IDLE")
        self.submodules += fsm

        fsm.act("IDLE",
            If(self.enable.storage==True,
            #If(~fifo.sink.ready,                   # FIFO is full
                NextValue(LedCount, fifo_size - 1), # word count
                NextValue(fifo.source.ready, 1),
                NextState("FIFOREAD")
            )
        )
        fsm.act("FIFOREAD",
            NextValue(GRBdata, fifo.source.data ),
            NextValue(fifo.source.ready, 0),        # ACK read
            NextValue(BitCount, dw - 1),            # Bit count
            NextState("LOADCOUNTER")
        )
        fsm.act("LOADCOUNTER",
            NextValue(self.DataPin, 1),             # Always start high
            If(GRBdata[dw - 1],                     # Always read last bit, cycled left later
                NextValue(PulseLen, T1H),
            ).Else(
                NextValue(PulseLen, T0H),
            ),
            NextState("HIGH")
        )
        fsm.act("HIGH",
            If(PulseLen == 0,
                If(GRBdata[dw - 1],
                    NextValue(PulseLen, T1L)
                ).Else(
                    NextValue(PulseLen, T0L)
                ),
                NextState("LOW")
            ).Else(
                NextValue(PulseLen, PulseLen - 1),
            )

        )
        fsm.act("LOW",
            NextValue(self.DataPin, 0),
            If(PulseLen == 0,
                NextValue(GRBdata, GRBdata << 1),   # Next bit (of GRB)
                NextState("NEXTBIT")
            ).Else(
                NextValue(PulseLen, PulseLen - 1),
            )
        )
        fsm.act("NEXTBIT",
            If(BitCount != 0,                       # Not yet done?
		NextValue(BitCount, BitCount - 1),
                NextState("LOADCOUNTER")
            ).Elif(LedCount != 0,                   # Next LED
		NextValue(LedCount, LedCount - 1),
                NextState("FIFOREAD"),
                NextValue(fifo.source.ready, 1),
            ).Else(
                NextValue(BitCount,0),
                NextValue(PulseLen, 2047),          # = 98us
                NextState("RST")
            )
        )
        fsm.act("RST",
            NextValue(PulseLen, PulseLen - 1),
            NextValue(self.enable.storage, 0),
            If(PulseLen == 0,
                NextState("IDLE")
            )
        )

