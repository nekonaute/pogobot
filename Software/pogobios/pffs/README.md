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

The local LiteX readers retain their original notices and add bounded 160-byte
input plus file-service hooks. Completion uses the same header. Native NOR,
protocol and full/minimal console tests compile the canonical pogolib sources.
See [PFFS file commands](../../readme.md#pffs-file-commands) for usage.
