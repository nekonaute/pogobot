# PFFS BIOS adapter

The filesystem and append-log core lives in [pogolib](../../pogolib/pffs/README.md).
Pogobios links it from `libpogobot.a`, as do SDK applications. There is no
second filesystem implementation in the BIOS.

The UART protocol in `transfer.c/.h` originated in `pogo-utils` commit
`ac6d558`, under `examples/flash_file/`. The matching host client is
`Software/pffs_transfer.py`, also packaged in the SDK's `tools` directory.
The PFT1 protocol and PFFS v4 storage layout are unchanged.

`../cmd_pffs.c` adapts file operations to native BIOS handlers, a shared page
cache and incremental service calls. `../console_input.c` serializes file
sessions against other commands and prohibits PFFS over IR. Command and transfer
entry points check catalogs first, so only `format YES` can initialize or replace
them. The lower-level library retains its existing create-time autoformat policy.

`load.c/.h` adds a robot-only `load <id|name>` controller installer. It validates
the stored raw file before changing the application slot, uses the existing page
cache, verifies every programmed page and the complete installed CRC, and commits
the boot marker last. Failed writes leave the image invalid. The source file is
read-only throughout installation; `run` remains a separate command. Native
console tests inject marker erase/write, sector erase, page write, source-read
and later-page corruption faults, including retry and 128 KiB boundary cases.

The local LiteX readers retain their original notices and add bounded 160-byte
input plus file-service hooks. Completion uses the same header. Native NOR,
protocol and full/minimal console tests compile the canonical pogolib sources.
See [PFFS file commands](../../readme.md#pffs-file-commands) for usage.

Defragmentation remains in pogolib for maintenance applications such as
`pogo-utils/examples/flash_file`; its command and service state are omitted
from Pogobios. Files still require contiguous extents when allocated.

The robot boot action `load_run <name>` is the narrow IR exception: it uses
exact name lookup, the existing verified controller installer and the same
session locks, then selects user image 1 only on success. Missing/invalid files
or installation faults show red without booting an older image. The normal
marker LED refresh skips this action so it cannot mask the red result. Remote
`rc_load_run <name>` reuses rc_start's 200 ms broadcasts and key cancellation;
ordinary file management and `load` remain UART-only.

`ir_file.c` sends stored files using a private user-packet command range
0x15..0x19. The shared decoder and GF tables live in `../ir_boot.c`; firmware
uploads retain their original packet type and command range. File START derives
its destination exclusively from checked PFFS allocation. Private indexed writer
helpers in pogolib erase the reserved extent and publish its padded-page CRC
through the existing catalog writer. The public sequential writer API and v4
layout are unchanged. The BIOS excludes UART writers/firmware updates while an
IR file is pending, including after timeout. `ir_file_abort` releases that lock.
Native tests run the actual sender/receiver against strict NOR, including the
full 1,408 KiB region, loss/FEC, source binding, restart and replacement faults.

Preparation STARTs carry a bounded fresh-attempt flag for the first three
copies; later keepalives cannot repeatedly erase a failed destination. This
transport flag is excluded from the stable metadata hash and resume identity.
Matching completed transfers are consumed before re-entering the receive loop,
so successful robots remain usable during other receivers' repair passes.
