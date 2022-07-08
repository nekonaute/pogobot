#!/usr/bin/env python3

# POGOBOT
# Copyright © 2022 Sorbonne Université ISIR
# This file is licensed under the Expat License, sometimes known as the MIT License.
# Please refer to file LICENCE for details.


import os
import sys
import argparse


from migen import *
from migen.genlib.resetsync import AsyncResetSynchronizer

from litex.build.lattice.programmer import IceStormProgrammer

from litex.soc.cores.ram import Up5kSPRAM
from litex.soc.cores.clock import iCE40PLL
from litex.soc.cores.cpu import CPUNone
from litex.soc.integration.soc_core import *
from litex.soc.integration.soc import SoCRegion
from litex.soc.integration.builder import *

from litex.soc.interconnect.csr import AutoCSR, CSRStatus, CSRStorage, CSRField, CSRAccess
from litex.soc.interconnect.wishbone import SRAM
from litex.soc.cores.gpio import GPIOOut, GPIOTristate
from migen.genlib.cdc import MultiReg
from litex.soc.doc import generate_docs, generate_svd
from litex.soc.cores.uart import UARTPHY,UARTPads,UARTWishboneBridge, UART

from rtl.sbled import SBLED
from rtl.romgen import RandomFirmwareROM, FirmwareROM
from rtl.sbwarmboot import SBWarmBoot

from litescope import LiteScopeAnalyzer

from ts4231 import PHYRX, PHYTX, IR_RX, IR_TX

from math import ceil

kB = 1024
mB = 1024*kB

class BaseSoC(SoCCore, AutoCSR):
    interrupt_map = {
        "timer0": 2,
        "ir_tx": 3,
        "ir_rx0": 4,
    }
    interrupt_map.update(SoCCore.interrupt_map)

    def __init__(self, bios_flash_offset,
                 pnr_seed=0,
                 debug_tx_01=False, debug_rx_01=False,
                 warmboot_offsets=None,
                 flashsize=0,
                 remocon=True,
                 **kwargs):

        debug_uart=False

        # Disable Integrated ROM/SRAM since too large for iCE40 and UP5K has specific SPRAM.
        kwargs["integrated_sram_size"] = 0
        kwargs["integrated_rom_size"]  = 0

        if (debug_uart):
            kwargs["uart_name"] = "crossover" # Debug over UART and keep the console
            # Bridge is added later

        # Original target use the 12MHz oscillator directly
        #sys_clk_freq = int(12e6)
        #from litex_boards.platforms import lattice_ice40up5k_evn
        sys_clk_freq = int(21e6)
        from platforms import lattice_ice40up5k_evn
        platform = lattice_ice40up5k_evn.Platform()
        from targets.lattice_ice40up5k_evn import _CRG
        #from litex_boards.targets.lattice_ice40up5k_evn import _CRG

        # SoCCore ----------------------------------------------------------------------------------
        SoCCore.__init__(self, platform, sys_clk_freq,
            ident          = "LiteX SoC on Lattice iCE40",
            **kwargs)

        # CPU --------------------------------------------------------------------------------------
        cpu_type = kwargs.get("cpu_type")
        cpu_variant = kwargs.get("cpu_variant")

        if (cpu_type == "vexriscv") and (cpu_variant == "lite"):
            self.cpu.use_external_variant("rtl/VexRiscv_Lite.v")

        # CRG --------------------------------------------------------------------------------------
        self.submodules.crg = _CRG(platform, sys_clk_freq)

        # 128KB SPRAM (used as SRAM) ---------------------------------------------------------------
        self.submodules.spram = Up5kSPRAM(size=128*kB)
        self.bus.add_slave("sram", self.spram.bus, SoCRegion(size=128*kB))

        # SPI Flash --------------------------------------------------------------------------------
        litespi=False
        if litespi:
            # New method using LiteSPI (but takes 300 more LCs) :
            from litespi.modules import N25Q032A
            from litespi.opcodes import SpiNorFlashOpCodes as Codes
            self.add_spi_flash(mode="1x", module=N25Q032A(Codes.READ_1_1_1), with_master=False )
        else:
            # Old litex SPI module supports memory-mapped reads, as well as a bit-banged mode
            # for doing flash writes or accessing IMU and ADC.
            from spi_flash import SpiFlash
            pads = platform.request("spiflash")
            self.submodules.spiflash = SpiFlash(pads, dummy=8, endianness=self.cpu.endianness)
            spiflash_region = SoCRegion(origin=self.mem_map.get("spiflash", None), size=flashsize)
            self.bus.add_slave(name="spiflash", slave=self.spiflash.bus, region=spiflash_region)

        # Add ROM linker region --------------------------------------------------------------------
        self.bus.add_region("rom", SoCRegion(
            origin = self.bus.regions["spiflash"].origin + bios_flash_offset,
            size   = 64*kB,
            linker = True)
        )
        self.cpu.set_reset_address(self.bus.regions["rom"].origin)

        spiflash_base = self.bus.regions["spiflash"].origin

        self.add_constant("SPIFLASH_PAGE_SIZE", 256)
        self.add_constant("SPIFLASH_SECTOR_SIZE", 0x10000)  # Default erase sector size in LiteX's spiflash.c

        # RGB LED ---------------------------------------------------------------------------------
        self.submodules.rgb = SBLED(platform.request("rgb_led"))
        #from litex.soc.cores.led import LedChaser
        #self.submodules.leds = LedChaser(
        #        pads = platform.request_all("user_led_n"),
        #        sys_clk_freq = sys_clkc_freq)

        # Reboot ----------------------------------------------------------------------------------
        iCE40_reboot = Signal()
        # Allow the user to reboot the ICE40
        self.submodules.reboot = SBWarmBoot(self, offsets=warmboot_offsets)
        iCE40_reboot = self.reboot.do_reset

        # Patch to speed up CSR read/write -----------------------------------
        # Modify litex/soc/cores/cpu/vexriscv/core.py : 
        #-        self.periph_buses     = [ibus, dbus]
        #+        self.periph_buses     = [dbus]

        if False:
            from litex.soc.interconnect import wishbone
            # Get ROM and SPRAM Slave interfaces and add a direct connection to CPU Instruction bus.
            rom   = self.bus.slaves.pop("spiflash")
            sram  = self.bus.slaves.pop("sram")
            self.bus.slaves["spiflash"] = wishbone.Interface()
            self.bus.slaves["sram"] = wishbone.Interface()

            self.submodules.bus_interconnect = wishbone.InterconnectShared(
            masters = [self.cpu.ibus, self.bus.slaves["spiflash"], self.bus.slaves["sram"]],
                slaves  = [(self.bus.regions[n].decoder(self.bus), s) for n, s in {"rom": rom, "sram": sram}.items()]
            )

        # Add a UART-Wishbone bridge -----------------------------------------
        if debug_uart:
            self.submodules.uart_bridge = UARTWishboneBridge(
                platform.request("serial"),
                sys_clk_freq,
                baudrate=115200)
            self.add_wb_master(self.uart_bridge.wishbone)

        if remocon == True:
            # Add constant to soc.h
            self.add_constant("REMOCON", "1")

        # PWM for the 3 motors
        PWM_enabled = False
        if (PWM_enabled == True):
            from pwm import COUNTER,PWM
            self.submodules.counter = COUNTER()
            self.submodules.motor_right = PWM(self.counter) # M1
            self.submodules.motor_left = PWM(self.counter)  # M2
            self.submodules.motor_middle = PWM(self.counter)# M3
            self.comb += self.platform.request("Motors",0 ).eq(self.motor_right.output)
            self.comb += self.platform.request("Motors",1 ).eq(self.motor_left.output)
            self.comb += self.platform.request("Motors",2 ).eq(self.motor_middle.output)


        # How many IR modules ?
        IR=1

        IR_fifo_tx = 512
        IR_fifo_rx = 512 # one for each receiver

        if IR>0:   # One RX module for each IR but one TX module for all 
            ir_i = 0
            ### TX
            tx_main = Signal(reset_less=True) # Single TX signal for 1 to 4 IR LEDs
            self.submodules.ir_phytx = PHYTX(tx=tx_main, debug_tx_01=debug_tx_01)
            self.submodules.ir_tx = IR_TX(self.ir_phytx,
                    tx_fifo_depth=IR_fifo_tx,
                    debug_tx_01=debug_tx_01,
                    )  # One TX module for all

            if remocon:
                self.comb += self.ir_phytx.remote.eq(self.ir_tx._remote.storage)
            else:
                self.comb += self.ir_phytx.remote.eq(0)

        ### RX
        IR_tx = []
        IR_rx = []
        for ir_i in range(IR):
            IR_tx.append(self.platform.request("ir_tx", ir_i))
            IR_rx.append(self.platform.request("TS4231", ir_i))

            setattr(self.submodules, 'ir_phyrx%d' % ir_i, PHYRX(debug_rx_01=debug_rx_01))
            setattr(self.submodules, 'ir_rx%d' % ir_i, IR_RX(getattr(self, 'ir_phyrx%d' % ir_i), rx_pads=IR_rx[ir_i],
                    rx_fifo_depth=IR_fifo_rx, debug_rx_01=debug_rx_01,
                    rx_fifo_rx_we=False,
                    ))
            # RX enabled only if not transmitting (unless specified with echo_cancel register bit)
            self.comb += getattr(self, 'ir_phyrx%d' % ir_i).rx_enable.eq(~self.ir_phytx.tx_busy
                           | ~getattr(self, 'ir_rx%d' % ir_i)._conf.fields.echo_cancel)
            # Connect tx_main (main TX signal) to H and L signals (going to TX transistors), through tx_mask
            self.comb += [ IR_tx[ir_i].H.eq(self.ir_tx._conf.fields.tx_mask[ir_i]
                            & getattr(self, 'ir_rx%d' % ir_i)._conf.fields.tx_power[1]
                            & tx_main),
                           IR_tx[ir_i].L.eq(self.ir_tx._conf.fields.tx_mask[ir_i]
                            & getattr(self, 'ir_rx%d' % ir_i)._conf.fields.tx_power[0]
                            & tx_main)]

            if not ( (cpu_type == "serv") | (cpu_type == None)):
                self.irq.add("ir_rx"+str(ir_i), use_loc_if_exists=True)

            # Connects RX threshold conf register bits
            if debug_rx_01:
                self.comb += [
                    getattr(self, 'ir_phyrx%d' % ir_i).rx_one.eq(getattr(self, 'ir_rx%d' % ir_i)._conf.fields.rx_one),
                    getattr(self, 'ir_phyrx%d' % ir_i).rx_zero.eq(getattr(self, 'ir_rx%d' % ir_i)._conf.fields.rx_zero) ]

        # Reboot if special signal detected
        if IR==1:
            self.comb += self.reboot.ir_reset.eq(self.ir_phyrx0.reboot)
        if IR==2:
            self.comb += self.reboot.ir_reset.eq(self.ir_phyrx0.reboot|self.ir_phyrx1.reboot)
        if IR==3:
            self.comb += self.reboot.ir_reset.eq(self.ir_phyrx0.reboot|self.ir_phyrx1.reboot|self.ir_phyrx2.reboot)
        if IR==4:
            self.comb += self.reboot.ir_reset.eq(self.ir_phyrx0.reboot|self.ir_phyrx1.reboot|self.ir_phyrx2.reboot|self.ir_phyrx3.reboot)

        # Options to yosys/nextpnr (from foboot)
        foboot_optim=True
        if foboot_optim:
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
                --pre-pack {build_name}_pre_pack.py --{architecture} --package {package} --timing-allow-fail",
                "icepack {build_name}.txt {build_name}.bin"
            ]

            # Add "-relut -dffe_min_ce_use 4" to the synth_ice40 command.
            # The "-reult" adds an additional LUT pass to pack more stuff in,
            # and the "-dffe_min_ce_use 4" flag prevents Yosys from generating a
            # Clock Enable signal for a LUT that has fewer than 4 flip-flops.
            # This increases density, and lets us use the FPGA more efficiently.
            platform.toolchain.yosys_template[2] += " -relut -abc2 -dffe_min_ce_use 4 -relut"
            #if use_dsp:
            platform.toolchain.yosys_template[2] += " -dsp"

            # Disable final deep-sleep power down so firmware words are loaded
            # onto softcore's address bus.
            platform.toolchain.build_template[2] = "icepack -s {build_name}.txt {build_name}.bin"

            # Allow us to set the nextpnr seed
            platform.toolchain.build_template[1] += " --seed " + str(pnr_seed)

            #if pnr_placer is not None:
            #   platform.toolchain.build_template[1] += " --placer {}".format(pnr_placer)

def make_multiboot_header(filename, boot_offsets=[160]):
    """
    ICE40 allows you to program the SB_WARMBOOT state machine by adding the following
    values to the bitstream, before any given image:

    [7e aa 99 7e]       Sync Header
    [92 00 k0]          Boot mode (k = 1 for cold boot, 0 for warmboot)
    [44 03 o1 o2 o3]    Boot address
    [82 00 00]          Bank offset
    [01 08]             Reboot
    [...]               Padding (up to 32 bytes)

    Note that in ICE40, the second nybble indicates the number of remaining bytes
    (with the exception of the sync header).

    The above construct is repeated five times:

    INITIAL_BOOT        The image loaded at first boot
    BOOT_S00            The first image for SB_WARMBOOT
    BOOT_S01            The second image for SB_WARMBOOT
    BOOT_S10            The third image for SB_WARMBOOT
    BOOT_S11            The fourth image for SB_WARMBOOT
    """
    while len(boot_offsets) < 5:
        boot_offsets.append(boot_offsets[0])

    with open(filename, 'wb') as output:
        for offset in boot_offsets:
            # Sync Header
            output.write(bytes([0x7e, 0xaa, 0x99, 0x7e]))

            # Boot mode
            output.write(bytes([0x92, 0x00, 0x00]))

            # Boot address
            output.write(bytes([0x44, 0x03,
                    (offset >> 16) & 0xff,
                    (offset >> 8)  & 0xff,
                    (offset >> 0)  & 0xff]))

            # Bank offset
            output.write(bytes([0x82, 0x00, 0x00]))

            # Reboot command
            output.write(bytes([0x01, 0x08]))

            for x in range(17, 32):
                output.write(bytes([0]))

def get_pogosoc_git_revision():
    try:
        r = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"],
                stderr=subprocess.DEVNULL)[:-1].decode("utf-8")
    except:
        r = "--------"
    return r

# Flash --------------------------------------------------------------------------------------------

def flash(firmware, target=None, bootloader=False, flashsize=0, spiflash_base=0):

    bitstream  = open("build/"+target+"/gateware/"+target+".bin",  "rb")

    if firmware=="bios":
        ice40_firmware = open("build/"+target+"/software/bios/bios.bin", "rb")
    if firmware=="pogobios":
        # Add git sha1 to generated 
        import re
        pogosoc_git_ver = get_pogosoc_git_revision()
        git_h_file = open("build/"+target+"/software/include/generated/git.h", "r+")
        done=False
        for line in git_h_file:
            if re.search("POGOSOC_GIT_SHA", line):
                git_h_file.write("#define POGOSOC_GIT_SHA1 \""+pogosoc_git_ver+"\"\n")
                done=True
        if not done:
            git_h_file.write("#define POGOSOC_GIT_SHA1 \""+pogosoc_git_ver+"\"\n")
        git_h_file.close()

        return_val = os.system("mkdir -p build/"+target+"/software/"+firmware+"; make -C pogolib clean all TARGET="+target+"&& make -C pogobios TARGET="+target)
        if return_val != 0:
            exit()
        ice40_firmware = open("build/"+target+"/software/"+firmware+"/"+firmware+".bin", "rb")

    if bootloader:
        image      = open("build/"+target+"/bootloader.bin", "wb")
        # Copy Multiboot Header
        multiboot_header_file  = open("build/"+target+"/gateware/multiboot-header.bin",  "rb")
        multiboot_header = multiboot_header_file.read()
        image.write(multiboot_header)
        bl_space=160
    else:
        image      = open("build/"+target+"/image.bin", "wb")
        bl_space=0

    # Copy bitstream 0x00000000
    #print("Copying bitstream...")
    for i in range(0x00000000, (0x0020000 - bl_space)):
        b = bitstream.read(1)
        if not b:
            image.write(0xff.to_bytes(1, "big"))
        else:
            image.write(b)
    # Copy bios 0x00020000
    #print("Copying iCE40 riscv firmware : "+firmware+" ...")
    for i in range(0x00000000, 0x00010000):
        b = ice40_firmware.read(1)
        if not b:
            image.write(0xff.to_bytes(1, "big"))
        else:
            image.write(b)
    image.close()

    # Generate the JSON file for litex-term serial-boot feature
    json = open("build/"+target+"/images.json", "w")
    json.write("{\n")
    if bootloader:
        address = spiflash_base
        name = "build/"+target+"/gateware/multiboot-header.bin"
        json.write(f'    "{name}": "{hex(address)}",\n')
        address += 0xA0
        name = bitstream.name
        json.write(f'    "{name}": "{hex(address)}",\n')
        name = ice40_firmware.name
        json.write(f'    "{name}": "{hex(spiflash_base+0x20000)}"')
    else:
        address = 0x40000 + spiflash_base
        name = bitstream.name
        json.write(f'    "{name}": "{hex(address)}",\n')
        address += 0x20000
        json.write(f'    "{ice40_firmware.name}": "{hex(address)}"')

    json.write("\n}\n")
    json.close()
    # Create symlink
    if os.access("images.json",os.R_OK):
        os.remove("images.json")
    os.symlink("build/"+target+"/images.json", "images.json")

    # Generate JSON to write only the software
    json = open("build/"+target+"/pogobios.json", "w")
    json.write("{\n")
    json.write(f'    "{ice40_firmware.name}": "{hex(0x60000+spiflash_base)}"')
    json.write("\n}\n")
    json.close()
    if os.access("pogobios.json",os.R_OK):
        os.remove("pogobios.json")
    os.symlink("build/"+target+"/pogobios.json", "pogobios.json")

    ice40_firmware.close()
    bitstream.close()

    # Write a file of the right size for flashrom tool
    if bootloader: # this means it will probably be located at address 0
        flashrom_image  = open("build/"+target+"/gateware/"+target+"_flashrom.bin",  "wb")
        image      = open("build/"+target+"/bootloader.bin", "rb")

        for i in range(0x0, flashsize):
            b = image.read(1)
            if not b:
                flashrom_image.write(0xff.to_bytes(1, "big"))
            else:
                flashrom_image.write(b)
        image.close()
        flashrom_image.close()
        print("Flashrom image available : build/"+target+"/gateware/"+target+"_flashrom.bin")

    prog = IceStormProgrammer()
    wait_for_usb_device("0403:6010")
    if bootloader:
        print("Flashing : build/"+target+"/bootloader.bin at address 0")
        rc = prog.flash(0x0, "build/"+target+"/bootloader.bin")
    else:
        print("build/"+target+"/image.bin generated")
        print("Do you want to flash ? (ctrl-C to stop)")
        input()
        print("Flashing : build/"+target+"/image.bin at address 0x40000")
        rc = prog.flash(0x40000, "build/"+target+"/image.bin")

    json.close()

# Build --------------------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="LiteX SoC on Lattice iCE40UP5K EVN Breakout board")
    parser.add_argument("--build", action="store_true", help="Build bitstream")
    parser.add_argument("--bootloader", action="store_true", help="Build bootloader instead of normal image")
    parser.add_argument("--remocon", action="store_true", help="Build a remote control")
    #parser.add_argument("--bios-flash-offset", default=0x20000, help="BIOS offset in SPI Flash", type=lambda x: int(x,0))
    parser.add_argument("--flash", default="pogobios", help="Add firmware (bios|pogobios) to image file")
    parser.add_argument("--flash_size", type=lambda x: int(x,0), help="Specify flash size")
    parser.add_argument("--seed", default=0, help="seed to use in nextpnr")
    parser.add_argument("--debug_tx_01", action="store_true", help="Puts size of zero and one in a register")
    parser.add_argument("--debug_rx_01", action="store_true", help="Puts thresholds for zero and one in a register")
    builder_args(parser)
    soc_core_args(parser)
    args = parser.parse_args()

    flashsize=0x400000 # 16Mb (2MB) on evn board
    target = "lattice_ice40up5k_evn"

    if args.flash_size:
        flashsize=args.flash_size   # Overwrite default flash size

    if args.bootloader:
        warmboot_offsets  = [
            0xA0,
            0xA0,
            0x40000,
            0xA0,
            0xA0 ]
        bios_flash_offset = 0x20000
    else:
        warmboot_offsets  = None
        bios_flash_offset = 0x60000 # Offset for the second bitstream (0x40000) + 0x20000 (rounded size of a bitstream)

    soc = BaseSoC(bios_flash_offset,
                  debug_tx_01=args.debug_tx_01, debug_rx_01=args.debug_rx_01,
                  pnr_seed=int(args.seed),
                  warmboot_offsets=warmboot_offsets,
                  flashsize=flashsize,
                  remocon=args.remocon,
                  **soc_core_argdict(args))
    # DEBUG with LiteScope
    if False:
        analyzer_signals = [
            soc.IR0.rx,
            soc.IR0_uart._rxtx.r,
        ]
        soc.submodules.analyzer = LiteScopeAnalyzer(analyzer_signals, 512 , clock_domain="sys")
        # second argument is the depth of the analyzer
        soc.add_csr("analyzer")

    builder = Builder(soc, **builder_argdict(args))
    builder.csr_csv="csr.csv"
    builder.build(run=args.build)

    variables = open("build/"+target+"/software/include/generated/variables.mak", "a")
    variables.write("\nSPIFLASH_BASE="+hex(soc.bus.regions["spiflash"].origin)+"\n")
    variables.write("ROM_BASE="+hex(soc.bus.regions["rom"].origin)+"\n")

    if args.bootloader:
        # Make Multiboot Header
        make_multiboot_header("build/"+target+"/gateware/multiboot-header.bin", warmboot_offsets)

    if args.doc:
        generate_docs(soc, "build/"+target+"/documentation")
        generate_svd(soc, "build/"+target+"/software")

    if args.flash:
        rc = flash(firmware=args.flash,
                    target=target,
                    bootloader=args.bootloader,
                    flashsize=flashsize,
                    spiflash_base = soc.bus.regions["spiflash"].origin)
        print(rc)
        return rc

if __name__ == "__main__":
    rc = main()
    if (rc != 0):
    #    print("Script exiting with error code " + str(rc))
        exit(rc)
