#!/usr/bin/env python3

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

from migen import *
from migen.genlib.record import Record
from migen.genlib.cdc import MultiReg
from migen.genlib.misc import WaitTimer

from litex.soc.interconnect.csr import *
from litex.soc.interconnect.csr_eventmanager import *
from litex.soc.interconnect import wishbone
from litex.soc.interconnect import stream

dw = 8 # Data Width
RX_ZERO = 14 #6 # Upper limits to compare
RX_ONE = 18 #10
RX_ZERO_MIN = 1 # Lower limit (Glitch?)

TX_ZERO = 6 #3 # Wait duration after a pulse
TX_ONE = 16 #8


def UARTPads():
    return Record([("tx", 1), ("rx", 1)])

# TS4231 PHY ----------------------------------------------------------------------------------------
class PHYRX(Module):
    def __init__(self, debug_rx_01=False, hard_reset=False):
        self.source = stream.Endpoint([("data", dw)])
        self.rx_raw = Signal(reset_less=True)       # Signals from IR_RX tristate module
        self.env_raw = Signal(reset_less=True)
        rx = Signal(reset_less=True)
        self.env = Signal(reset_less=True)
        self.specials += MultiReg(self.rx_raw, rx, odomain="sys")
        self.specials += MultiReg(self.env_raw, self.env, odomain="sys")
        rx_r = Signal(reset_less=True)              # rx one cycle before
        rx_reg = Signal(dw, reset_less=True)
        rx_bitcount = Signal(max=31, reset_less=True)
        rx_cnt = Signal(max=RX_ONE+RX_ZERO+1, bits_sign=None, reset_less=True)

        rx_data = Signal(dw, reset_less=True)       # Data sent to FIFO
        rx_data = self.source.data

        rx_gap = Signal(reset_less=True)            # =0 if short gap, =1 if long gap
        rx_trig = Signal(reset_less=True)           # triggers the read of gap = bit read

        if debug_rx_01:
            self.rx_zero = Signal(5)                # Limit for a 0
            self.rx_one = Signal(5)                 # Limit for a 1 (must be > rx_zero)
        else:
            self.rx_zero = RX_ZERO
            self.rx_one = RX_ONE
        self.rx_zero_min = RX_ZERO_MIN

        HEADER_MIN = 3                              # Valid min size of header 
        RX_TIMEOUT = RX_ONE + RX_ZERO

        if hard_reset:
            REBOOT_MSG = 0x37132142DEFAFECA
            reboot_msg = Signal(64, reset_less=True)# Special message to send to reboot
            reboot_cnt = Signal(max=8)

        self.rx_enable = Signal(reset_less=True)    # Disable when emitting locally

        self.reboot = Signal()

        # # #

        self.submodules.fsm = FSM(reset_state="START")

        self.sync += [
            If(~self.env & self.rx_enable,          # Env is normally high
               rx_r.eq(rx),
               If(rx & ~rx_r,                       # Transition detected, rising edge
                  rx_cnt.eq(0),
                  If( rx_cnt < self.rx_zero ,       # Transistion valid, short change
                      #If( rx_cnt > self.rx_zero_min ,
                          rx_gap.eq(0),
                          rx_trig.eq(1),
                      #)
                    ).Elif( (rx_cnt < self.rx_one ),# Transistion valid, long change
                      rx_gap.eq(1),
                      rx_trig.eq(1),
                      ) # TODO: Add Else here to detect faults ?
               ).Else(                              # No rising edge
                    rx_trig.eq(0),
                    If( rx_cnt != RX_TIMEOUT,       # Stop counting if timeout reached
                        rx_cnt.eq(rx_cnt + 1),
                    ),
               )
            )
        ]

        self.fsm.act("START",
            If(rx_trig,
                If(rx_gap,                          # One detected
                    NextValue(rx_bitcount, 0),
                    If( rx_bitcount > HEADER_MIN,   # Header detected ( n zeros )
                        NextState("WORD"),
                    )
                ).Else(                             # Count the zeros (ignore early ones, glitches)
                    NextValue(rx_bitcount, rx_bitcount + 1)
                )
            )
        )

        self.fsm.act("WORD",
            NextValue(self.source.valid, 0),
            If(rx_trig,
                NextValue(rx_reg, Cat(rx_reg[1:], rx_gap)),
                If(rx_bitcount == ( dw - 1 ),
                    NextState("NEXT_WORD"),
                    NextValue(rx_bitcount, 0),
                ).Else(
                    NextValue(rx_bitcount, rx_bitcount + 1),
                ),
            ).Elif( rx_cnt == RX_TIMEOUT,
                NextValue(rx_bitcount, 0),
                NextState("START"),
            ),
            If(self.env,                            # Transmission aborted, ignore the last received bits
                NextValue(rx_bitcount, 0),
                NextState("START")
            )
        )

        if hard_reset:
            self.fsm.act("NEXT_WORD",
                NextValue(rx_data, rx_reg),
                NextValue(self.source.valid, 1),    # Send data to FIFO/wishbone bus
                NextState("WORD"),
                If(rx_reg == reboot_msg[0:8],
                    NextValue(reboot_cnt, reboot_cnt + 1),
                    NextValue(reboot_msg, reboot_msg[8:]),
                    If( reboot_cnt == 7 ,
                        NextValue(self.reboot,1),
                    )
                ).Else(
                    NextValue(reboot_cnt, 0),
                    NextValue(reboot_msg, REBOOT_MSG),
                ),
            )
        else:
            self.fsm.act("NEXT_WORD",
                NextValue(rx_data, rx_reg),
                NextValue(self.source.valid, 1),    # Send data to FIFO/wishbone bus
                NextState("WORD"),
            )

class PHYTX(Module):
    def __init__(self, tx=None, debug_tx_01=False):
        self.sink = stream.Endpoint([("data", dw)])

        self.tx = tx
        tx_reg = Signal(dw, reset_less=True)
        tx_bitcount = Signal(max=31, reset_less=True)
        tx_wait = Signal(5, reset_less=True)        # Wait n cycles
        self.tx_busy = Signal(reset_less=True)      # Used to disable local reception
        self.trig = Signal()
        self.rsttrig = Signal()
        TX_TIMEOUT = 31
        HEADER = 10

        # # #

        # clock cycles for a 0 and a 1
        if debug_tx_01:
            self.tx_one = Signal(5)
            self.tx_zero = Signal(5)
        else:
            self.tx_zero=TX_ZERO
            self.tx_one=TX_ONE

        self.submodules.fsm = FSM(reset_state="STANDBY")

        self.comb += self.tx_busy.eq(~ (self.fsm.ongoing("STANDBY") |
                                        self.fsm.ongoing("END")))

        self.sync += [
            If(self.tx_busy,
                If( tx_wait <= 2,
                    tx.eq(1),                       # Pulse @sys_clk/2
                ).Else(
                    tx.eq(0)
                ),
                If( tx_wait != 0,
                    tx_wait.eq(tx_wait - 1),        # tw_wait can be changed only if == 0
                )                   
            ).Else(
                tx.eq(0)
            )
        ]
        self.fsm.act("STANDBY",
            If(self.sink.valid & self.trig,
                NextValue(tx_wait, 1),
                NextValue(tx_reg, self.sink.data),
                NextValue(tx_bitcount, 0),
                NextState("HEADER"),
                NextValue(self.rsttrig, 1),
                NextValue(tx_bitcount, HEADER)
            )
        )

        self.fsm.act("HEADER",
            NextValue(self.rsttrig, 0),
            If(tx_wait == 0,
                If( tx_bitcount == 0,
                    NextState("NEXT_WORD"),
                    NextValue(tx_bitcount, -1),
                    NextValue(tx_wait, self.tx_one),
                ).Else(
                    NextValue(tx_bitcount, tx_bitcount - 1),
                    NextValue(tx_wait, self.tx_zero)
                )
            )
        )

        self.fsm.act("WORD",
            NextValue(self.sink.ready, 0),          # sink.ready connected to txfifo
            If(tx_wait == 0,
                If(tx_reg[0],
                   NextValue(tx_wait, self.tx_one), # The bit value to send sets the delay to wait
                ).Else(
                   NextValue(tx_wait, self.tx_zero),
                ),
                NextValue(tx_reg, Cat(tx_reg[1:], 0)),   # Shift bits
                If(tx_bitcount == ( dw - 1 ),       # last bit sent
                    NextValue(tx_bitcount, 0),
                ).Else(
                    NextValue(tx_bitcount, tx_bitcount + 1),
                ),
                If((tx_bitcount == ( dw - 1)) & (~self.sink.valid),
                    NextState("STANDBY"),           # Else, stop transmission
                )
            ),
            If(tx_wait == 2,                        # prepare
                If(tx_bitcount == ( dw - 1 ),       # last bit being sent
                    If(self.sink.valid,             # == _rxtx.re
                        NextValue(tx_reg, self.sink.data),  # Copy new word to register if available
                        NextState("NEXT_WORD"),
                    )
                )
            )
        )

        self.fsm.act("NEXT_WORD",
            NextValue(self.sink.ready, 1),          # Byte already copied, ask for next one to be ready
            NextState("WORD"),
        )

        #self.fsm.act("END", # Wait some time to break transmission 
        #    If(tx_bitcount < TX_TIMEOUT,
        #        NextValue(tx_bitcount, tx_bitcount + 1),
        #    ).Else(
        #        NextValue(tx_bitcount, 0),
        #        NextState("STANDBY")
        #    )
        #)


class IR_TX(Module, AutoCSR): # Only one instance of this module
    def __init__(self, phy=None,
            tx_fifo_depth = 16,
            debug_tx_01   = False,
            tx_phy_cd     = "sys"):
        self._tx      = CSR(dw)
        self._txfull  = CSRStatus()

        #self.submodules.ev = EventManager()
        #self.ev.tx         = EventSourceProcess()
        #self.ev.finalize()

        self._txempty = CSRStatus()
        self._rts     = CSRStatus()

        # # #

        self.source = stream.Endpoint([("data", dw)])

        # PHY
        if phy is not None:
            self.comb += [
                self.source.connect(phy.sink)
            ]

        # TX
        self.tx_fifo = tx_fifo = stream.SyncFIFO([("data", dw)], tx_fifo_depth, buffered=True)
        self.submodules += tx_fifo

        fields=[
            CSRField("tx_mask", size=4, reset=0xF, description="Select which IR emitter to use"),
            CSRField("tx_trig", size=1, reset=0, pulse=True, description="Write 1 to send what's in the FIFO")
        ]
        if debug_tx_01:
            fields += [
                CSRField("tx_zero", size=5, reset=TX_ZERO, access=CSRAccess.ReadWrite, description="Number of clocks cycles to wait to send a 0"),
                CSRField("tx_one", size=5, reset=TX_ONE, access=CSRAccess.ReadWrite, description="Number of clocks cycles to wait to send a 1"),
            ]
        self._conf = CSRStorage(32, fields=fields)

        self.comb += [
            tx_fifo.sink.valid.eq(self._tx.re),
            tx_fifo.sink.data.eq(self._tx.r),
            self._txfull.status.eq(~tx_fifo.sink.ready),    # sink.ready == fifo not full
            self._txempty.status.eq(~tx_fifo.source.valid), # source.valid == fifo not empty
            tx_fifo.source.connect(self.source),
            # Generate TX IRQ when tx_fifo becomes non-full
            # self.ev.tx.trigger.eq(~tx_fifo.sink.ready)
            #phy.trig.eq(self._conf.fields.tx_trig)
        ]

        if debug_tx_01:
            self.comb += [
                phy.tx_zero.eq(self._conf.fields.tx_zero),
                phy.tx_one.eq(self._conf.fields.tx_one)
                ]

class IR_RX(Module, AutoCSR):   # One instance of this module per transmitter
    def __init__(self, phy=None,
            rx_fifo_depth = 16,
            rx_fifo_rx_we = True,
            rx_phy_cd     = "sys",
            rx_pads       = None,
            debug_rx_01   = False ):
        self._rx      = CSR(dw)
        self._rxempty = CSRStatus()

        self.submodules.ev = EventManager()
        self.ev.rx = EventSourceProcess() #edge="rising")
        self.ev.finalize()

        self._rxfull  = CSRStatus()
        # Signals from TS4231 after tristate:
        self.rx = Signal(reset_less=True)
        self.env = Signal(reset_less=True)

        # # #

        self.sink   = stream.Endpoint([("data", dw)])

        # PHY
        if phy is not None:
            self.comb += [
                phy.source.connect(self.sink),
            ]

        # RX
        self.rx_fifo = rx_fifo = stream.SyncFIFO([("data", dw)], rx_fifo_depth, buffered=True)
        self.submodules += rx_fifo

        self.comb += [
            # Sink --> FIFO.
            self.sink.connect(rx_fifo.sink),
            # FIFO --> CSR.
            self._rx.w.eq(rx_fifo.source.data),
            rx_fifo.source.ready.eq(self.ev.rx.clear | (rx_fifo_rx_we & self._rx.we)),
            # Status.
            self._rxempty.status.eq(~rx_fifo.source.valid),
            self._rxfull.status.eq(~rx_fifo.sink.ready),
            # IRQ (When FIFO becomes non-empty).
            self.ev.rx.trigger.eq(~rx_fifo.source.valid)
        ]

        self._in  = CSRStatus(2, fields=[
            CSRField("d_r", description= "Status of data"),
            CSRField("e_r", description=" Status of envelope")
        ])
        fields=[
            CSRField("d_oe", description="Data output enable"),
            CSRField("e_oe", description="Enveloppe output enable"),
            CSRField("d_w", description="Sets the value of Data if d_oe enabled"),
            CSRField("e_w", description="Sets the value of Envelope if e_oe enabled"),
            CSRField("echo_cancel", reset=1, description="Inhibit reception of what is sent localy"),
            CSRField("tx_power", size=2, reset=1, access=CSRAccess.ReadWrite, description="Transmition power, 0 is off, 3 is the maximum"),
            ]
        if debug_rx_01:
            fields += [
                CSRField("rx_zero", size=5, reset=RX_ZERO, access=CSRAccess.ReadWrite, description="Short pulse max size for a zero"),
                CSRField("rx_one", size=5, reset=RX_ONE, access=CSRAccess.ReadWrite, description="Long pulse max size for a one"),
            ]
        self._conf = CSRStorage(32, fields=fields)

        # Tristates to configure TS4231
        _D = TSTriple()
        self.specials += _D.get_tristate(rx_pads.D)
        self.comb += _D.oe.eq(self._conf.fields.d_oe)
        self.comb += _D.o.eq(self._conf.fields.d_w)
        self.specials += MultiReg(_D.i, self._in.fields.d_r)

        _E = TSTriple()
        self.specials += _E.get_tristate(rx_pads.E)
        self.comb += _E.oe.eq(self._conf.fields.e_oe)
        self.comb += _E.o.eq(self._conf.fields.e_w)
        self.specials += MultiReg(_E.i, self._in.fields.e_r)

        self.comb += {
                phy.rx_raw.eq(self._in.fields.d_r),
                phy.env_raw.eq(self._in.fields.e_r),
                }

if __name__ == "__main__":
    from migen.fhdl.verilog import convert
    from litex.soc.interconnect.wishbone import SRAM
    convert(PHYTX(sram=SRAM(8,512))).write("PHYTX.v")
    convert(TS4231PHYRX()).write("TS4231PHYRX.v")
