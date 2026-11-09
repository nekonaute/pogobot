#!/usr/bin/env python3
"""Build production PFFS/console code against NOR flash and UART test doubles."""
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    tests = Path(__file__).resolve().parent
    bios = tests.parent / "pogobios"
    lib = tests.parent / "pogolib"
    with tempfile.TemporaryDirectory(prefix="pogobios-pffs-test-") as name:
        temp = Path(name)
        for header in ("pogobot.h", "spi.h", "uart.h", "sfl.h",
                       "generated/mem.h", "generated/soc.h", "generated/csr.h",
                       "libbase/spiflash.h"):
            path = temp / header
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(f'#include "{tests / "pffs_test_platform.h"}"\n')
        (temp / "complete.h").write_text("int complete(char *, char **);\n")
        # Production BIOS retains the same pointer table through its linker script.
        linker = temp / "commands.ld"
        linker.write_text("SECTIONS { .bios_cmd : { __bios_cmd_start = .; "
                          "KEEP(*(.bios_cmd)) __bios_cmd_end = .; } } INSERT AFTER .data;\n")
        flags = ["cc", "-std=c11", "-Os", "-g", "-flto", "-Wall",
                 "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                 "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                 "-fno-omit-frame-pointer", "-DREAL_ROBOT", "-I", str(temp), "-I", str(bios), "-I", str(lib)]
        core = [str(p) for p in sorted((lib / "pffs").glob("flash_*.c"))]
        for source in ("pffs_nor_test.c", "pffs_log_nor_test.c"):
            binary = temp / Path(source).stem
            subprocess.run([*flags, str(tests / source), *core, "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, timeout=60)
        for mini in (False, True):
            binary = temp / ("console-mini" if mini else "console")
            config = ["-DTERM_MINI", "-DTERM_NO_HIST", "-DTERM_NO_COMPLETE"] if mini else []
            sources = [str(tests / "pffs_console_test.c"), str(bios / "cmd_pffs.c"),
                       str(bios / "console_input.c"), str(bios / "pffs/transfer.c"),
                       str(bios / "pffs/load.c"),
                       str(bios / ("readline_simple.c" if mini else "readline.c"))]
            subprocess.run([*flags, *config, *sources, *core, "-Wl,-T," + str(linker),
                            "-o", str(binary)], check=True)
            result = subprocess.run([str(binary), "--selftest"], check=True,
                                    capture_output=True, text=True, timeout=60)
            print(result.stdout.splitlines()[-1], "(minimal)" if mini else "(full)", flush=True)
            if not mini:
                subprocess.run([sys.executable, str(tests / "pffs_transfer_checks.py"),
                                str(binary)], check=True, timeout=120)


if __name__ == "__main__":
    main()
