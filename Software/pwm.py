# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

from migen import *

from litex.soc.interconnect.csr import *
from litex.soc.integration.doc import AutoDoc,ModuleDoc


class COUNTER(Module):
    def __init__(self, width=10):
        self.width = width
        cnt = self.cnt = Signal(width, reset_less=True)
        period = 2**width # Fixed period to save space

        # # #

        self.sync += cnt.eq(cnt+1)

class PWM(Module, AutoCSR, AutoDoc, ModuleDoc):
    """
    Simple PWM class with fixed period.

    Usage:
######

#.  Write to _width register a value from 0 to 1023. 0 to stop it.

    Input:
######

    :_width: New PWM value (10 bits)

    Output:
######

    :output:    Output (connect to output pin)
    """
    def __init__(self, cnt):
        self.output = Signal()
        self._width = CSRStorage(cnt.width)

        # # #

        width = self._width.storage

        self.sync += \
            {
            If(cnt.cnt < width,
                self.output.eq(1)
            ).Else(
                self.output.eq(0)
            ),
            }
