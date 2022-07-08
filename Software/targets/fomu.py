#!/usr/bin/env python3

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License. 
# Please refer to file LICENCE for details.

# This file is Copyright (c) 2019 Sean Cross <sean@xobs.io>
# This file is Copyright (c) 2018 David Shah <dave@ds0.me>
# License: BSD

import os
import argparse

from migen import *
from migen.genlib.resetsync import AsyncResetSynchronizer

from litex.soc.cores.ram import Up5kSPRAM
from litex.soc.integration.soc_core import SoCCore
from litex.soc.integration.builder import Builder, builder_argdict, builder_args
from litex.soc.integration.soc_core import soc_core_argdict, soc_core_args
from litex.soc.integration.doc import AutoDoc
from litex.soc.cores.clock import iCE40PLL
import sys

import os, shutil, subprocess

# CRG ----------------------------------------------------------------------------------------------

class _CRG(Module, AutoDoc):
    """Fomu Clock Resource Generator

    Fomu is a USB device, which means it must have a 12 MHz clock.  Valentyusb
    oversamples the clock by 4x, which drives the requirement for a 48 MHz clock.
    The ICE40UP5k is a relatively low speed grade of FPGA that is incapable of
    running the entire design at 48 MHz, so the majority of the logic is placed
    in the 12 MHz domain while only critical USB logic is placed in the fast
    48 MHz domain.

    Fomu has a 48 MHz crystal on it, which provides the raw clock input.  This
    signal is fed through the ICE40 PLL in order to divide it down into a 12 MHz
    signal and keep the clocks within 1ns of phase.  Earlier designs used a simple
    flop, however this proved unreliable when the FPGA became very full.

    The following clock domains are available on this design:

    +---------+------------+---------------------------------+
    | Name    | Frequency  | Description                     |
    +=========+============+=================================+
    | usb_48  | 48 MHz     | Raw USB signals and pulse logic |
    +---------+------------+---------------------------------+
    | usb_12  | 12 MHz     | USB control logic               |
    +---------+------------+---------------------------------+
    | sys     | 12 MHz     | System CPU and wishbone bus     |
    +---------+------------+---------------------------------+
    """
    def __init__(self, platform, sys_clk_freq):
        #assert sys_clk_freq == 12e6
        self.rst = Signal()
        self.clock_domains.cd_sys = ClockDomain()
        self.clock_domains.cd_por    = ClockDomain(reset_less=True)
        #self.clock_domains.cd_usb_12 = ClockDomain()
        self.clock_domains.cd_clk20 = ClockDomain()
        self.clock_domains.cd_usb_48 = ClockDomain()
        
        # # #

        # Clk/Rst
        clk48 = platform.request("clk48")
        platform.add_period_constraint(clk48, 1e9/48e6)

        # Power On Reset
        por_count = Signal(16, reset=2**16-1)
        por_done  = Signal()
        self.comb += self.cd_por.clk.eq(ClockSignal("usb_48"))
        self.comb += por_done.eq(por_count == 0)
        self.sync.por += If(~por_done, por_count.eq(por_count - 1))

        # USB PLL
        self.submodules.pll = pll = iCE40PLL()
        self.comb += pll.reset.eq(self.rst)
        pll.clko_freq_range = ( 12e6,  275e9) # FIXME: improve iCE40PLL to avoid lowering clko_freq_min.
        pll.register_clkin(clk48, 48e6)
        #pll.create_clkout(self.cd_usb_12, 12e6, with_reset=False) #12MHz for USB
        pll.create_clkout(self.cd_clk20, 20e6, with_reset=False) # 20MHZ for sys
        self.comb += self.cd_usb_48.clk.eq(clk48)
        self.specials += AsyncResetSynchronizer(self.cd_clk20, ~por_done | ~pll.locked)
        #self.specials += AsyncResetSynchronizer(self.cd_usb_12, ~por_done | ~pll.locked)
        self.specials += AsyncResetSynchronizer(self.cd_usb_48, ~por_done | ~pll.locked)

        # Sys Clk
        #self.sync.usb_48 += self.cd_sys.clk.eq(~self.cd_sys.clk) # 24MHz clock
        #self.comb += self.cd_sys.clk.eq(self.cd_usb_12.clk) # 12MHz clock
        self.comb += self.cd_sys.clk.eq(self.cd_clk20.clk) # 20MHz clock
        self.specials += AsyncResetSynchronizer(self.cd_sys, ~por_done | ~pll.locked)
        # add constraint for sys
        #platform.add_period_constraint(self.cd_sys.clk, 1e9/20e6) # 20MHz 
        #platform.add_period_constraint(self.cd_sys.clk, 1e9/12e6)

# BaseSoC ------------------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    """A SoC on Fomu, optionally with a softcore CPU"""

    # Create a default CSR map to prevent values from getting reassigned.
    # This increases consistency across litex versions.
    SoCCore.csr_map = {
        "ctrl":           0,  # provided by default (optional)
        "crg":            1,  # user
        "uart_phy":       2,  # provided by default (optional)
        "uart":           3,  # provided by default (optional)
        "identifier_mem": 4,  # provided by default (optional)
        "timer0":         5,  # provided by default (optional)
        "cpu_or_bridge":  8,
        "usb":            9,
        "picorvspi":      10,
        "touch":          11,
        "reboot":         12,
        "rgb":            13,
        "version":        14,
    }

    # Statically-define the memory map, to prevent it from shifting across
    # various litex versions.
    SoCCore.mem_map = {
        "rom":      0x00000000,  # (default shadow @0x80000000)
        "sram":     0x10000000,  # (default shadow @0xa0000000)
        "spiflash": 0x20000000,  # (default shadow @0xa0000000)
        "main_ram": 0x40000000,  # (default shadow @0xc0000000)
        "csr":      0xe0000000,  # (default shadow @0x60000000)
    }

    def __init__(self, board,
        pnr_placer="heap", pnr_seed=0, usb_core="dummyusb", usb_bridge=False,
        use_dsp=True, **kwargs):
        """Create a basic SoC for Fomu.

        Create a basic SoC for Fomu, including a 48 MHz and 12 MHz clock
        domain called `usb_48` and `usb_12`.  The `sys` frequency will
        run at 12 MHz.

        The USB core will optionally have a bridge to the Wishbone bus.

        Args:
            board (str): Which Fomu board to build for: pvt, evt, or hacker
            pnr_placer (str): Which placer to use in nextpnr
            pnr_seed (int): Which seed to use in nextpnr
            usb_core (str): The name of the USB core to use, if any: dummyusb, epfifo, eptri
            usb_bridge (bool): Whether to include a USB-to-Wishbone bridge
        Raises:
            ValueError: If either the `usb_core` or `board` are unrecognized
        Returns:
            Newly-constructed SoC
        """
        if board == "pvt":
            from litex_boards.platforms.fomu_pvt import Platform
        elif board == "hacker":
            from litex_boards.platforms.fomu_hacker import Platform
        elif board == "evt":
            from litex_boards.platforms.fomu_evt import Platform
        elif board == "pogo":
            #from fomu_evt import Platform
            from platforms.fomu_pogobot import Platform
        else:
            raise ValueError("unrecognized fomu board: {}".format(board))
        platform = Platform()

        if "cpu_type" not in kwargs:
            kwargs["cpu_type"] = None
            kwargs["cpu_variant"] = None

        clk_freq = int(12e6)

        if "with_uart" not in kwargs:
            kwargs["with_uart"] = False

        if "with_ctrl" not in kwargs:
            kwargs["with_ctrl"] = False

        kwargs["integrated_sram_size"] = 0
        SoCCore.__init__(self, platform, clk_freq, **kwargs)

        self.submodules.crg = _CRG(platform)

        # UP5K has single port RAM, which is a dedicated 128 kilobyte block.
        # Use this as CPU RAM.
        spram_size = 128*1024
        self.submodules.spram = up5kspram.Up5kSPRAM(size=spram_size)
        self.register_mem("sram", self.mem_map["sram"], self.spram.bus, spram_size)

        if usb_core is not None:
            # Add USB pads.  We use DummyUsb, which simply enumerates as a USB
            # device.  Then all interaction is done via the wishbone bridge.
            usb_pads = platform.request("usb")
            usb_iobuf = usbio.IoBuf(usb_pads.d_p, usb_pads.d_n, usb_pads.pullup)
            if usb_core == "dummyusb":
                self.submodules.usb = dummyusb.DummyUsb(usb_iobuf, debug=usb_bridge)
            elif usb_core == "epfifo":
                self.submodules.usb = epfifo.PerEndpointFifo(usb_iobuf, debug=usb_bridge)
            elif usb_core == "eptri":
                self.submodules.usb = eptri.TriEndpointInterface(usb_iobuf, debug=usb_bridge)
            else:
                raise ValueError("unrecognized usb_core: {}".format(usb_core))
            if usb_bridge:
                self.add_wb_master(self.usb.debug_bridge.wishbone)

        # Override default LiteX's yosys/build templates
        assert hasattr(platform.toolchain, "yosys_template")
        assert hasattr(platform.toolchain, "build_template")
        platform.toolchain.yosys_template = [
            "{read_files}",
            "attrmap -tocase keep -imap keep=\"true\" keep=1 -imap keep=\"false\" keep=0 -remove keep=0",
            "synth_ice40 -json {build_name}.json -top {build_name}",
        ]
        platform.toolchain.build_template = [
            "yosys -q -l {build_name}.rpt {build_name}.ys",
            "nextpnr-ice40 --json {build_name}.json --pcf {build_name}.pcf --asc {build_name}.txt \
            --pre-pack {build_name}_pre_pack.py --{architecture} --package {package}",
            "icepack {build_name}.txt {build_name}.bin"
        ]

        # Add "-relut -dffe_min_ce_use 4" to the synth_ice40 command.
        # The "-reult" adds an additional LUT pass to pack more stuff in,
        # and the "-dffe_min_ce_use 4" flag prevents Yosys from generating a
        # Clock Enable signal for a LUT that has fewer than 4 flip-flops.
        # This increases density, and lets us use the FPGA more efficiently.
        platform.toolchain.yosys_template[2] += " -relut -abc2 -dffe_min_ce_use 4 -relut"
        if use_dsp:
            platform.toolchain.yosys_template[2] += " -dsp"

        # Disable final deep-sleep power down so firmware words are loaded
        # onto softcore's address bus.
        platform.toolchain.build_template[2] = "icepack -s {build_name}.txt {build_name}.bin"

        # Allow us to set the nextpnr seed
        platform.toolchain.build_template[1] += " --seed " + str(pnr_seed)

        if pnr_placer is not None:
            platform.toolchain.build_template[1] += " --placer {}".format(pnr_placer)


class USBSoC(BaseSoC):
    """A SoC for Fomu with interrupts for a softcore CPU"""

    interrupt_map = {
        "usb": 3,
    }
    interrupt_map.update(SoCCore.interrupt_map)


# Build --------------------------------------------------------------------------------------------

def add_dfu_suffix(fn):
    fn_base, _ext = os.path.splitext(fn)
    fn_dfu = fn_base + '.dfu'
    shutil.copyfile(fn, fn_dfu)
    subprocess.check_call(['dfu-suffix', '--pid', '1209', '--vid', '5bf0', '--add', fn_dfu])

def main():
    parser = argparse.ArgumentParser(description="LiteX SoC on Fomu")
    parser.add_argument("--build",  action="store_true", help="Build bitstream")
    parser.add_argument("--board",  choices=["pogo", "evt", "pvt", "hacker"], required=True, help="Build for a particular hardware board")
    parser.add_argument("--seed",   default=0, help="Seed to use in Nextpnr")
    parser.add_argument("--placer", default="heap", choices=["sa", "heap"], help="Which placer to use in Nextpnr")
    builder_args(parser)
    soc_core_args(parser)
    args = parser.parse_args()

    soc = BaseSoC(board=args.board, pnr_placer=args.placer, pnr_seed=args.seed, debug=True, **soc_core_argdict(args))
    builder = Builder(soc, **builder_argdict(args))
    builder.build(run=args.build)

if __name__ == "__main__":
    main()
