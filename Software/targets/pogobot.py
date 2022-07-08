#!/usr/bin/env python3

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License.
# Please refer to file LICENCE for details.

from migen import *
from migen.genlib.resetsync import AsyncResetSynchronizer

from litex.soc.cores.clock import iCE40PLL

class _CRG(Module):
   def __init__(self, platform, sys_clk_freq, PAD=False):
        self.rst = Signal()
        self.clock_domains.cd_sys    = ClockDomain()
        self.clock_domains.cd_por    = ClockDomain(reset_less=True)

        # # #

        # Clk/Rst
        clk48 = platform.request("clk48")
        platform.add_period_constraint(clk48, 1e9/48e6)

        # Power On Reset
        por_count = Signal(16, reset=2**16-1)
        por_done  = Signal()
        self.comb += self.cd_por.clk.eq(ClockSignal())
        self.comb += por_done.eq(por_count == 0)
        self.sync.por += If(~por_done, por_count.eq(por_count - 1))

        # PLL
        if PAD:
            self.submodules.pll = pll = iCE40PLL(primitive="SB_PLL40_PAD")
        else:
            self.submodules.pll = pll = iCE40PLL()
        self.comb += pll.reset.eq(self.rst)
        pll.clko_freq_range = ( 12e6,  275e9) # FIXME: improve iCE40PLL to avoid lowering clko_freq_min.
        pll.register_clkin(clk48, 48e6)
        pll.create_clkout(self.cd_sys, sys_clk_freq, with_reset=False) # 21MHZ for sys


        self.specials += AsyncResetSynchronizer(self.cd_sys, ~por_done | ~pll.locked)

        # add constraint for sys
        platform.add_period_constraint(self.cd_sys.clk, 1e9/sys_clk_freq)
