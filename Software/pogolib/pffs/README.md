# PFFS in pogolib

This directory is the maintained PFFS v4 file and append-log implementation.
It originated in `pogo-utils` commit `ac6d558`. The persistent catalog, function
signatures and page geometry are unchanged. Pogobios and hardware applications
link these objects from `libpogobot.a`; no neighboring checkout is needed to
build the firmware. Unused functions are discarded at the final link.

Include `<pogobot.h>` for the full SDK API, or `<pffs/flash_file.h>` and
`<pffs/flash_log.h>` for just the filesystem declarations. The feature macro
`POGOBOT_PFFS_API_VERSION` is 4. The SDK also distributes this directory under
`src/pffs` so native consumers can compile the same implementation.

Hardware uses the SDK page API and real 4 KiB SPI erases by default. Only a
native Pogosim build defines `POGOBOT_PFFS_SIMULATOR`, which selects
`pogobase.h` and emulates sector erase with page writes. Never define that
macro for a robot build. There is no heap allocation or floating-point code.

The functions are synchronous and do not lock the flash peripheral. Applications
must serialize access. Creation retains its existing autoformat behavior for
unformatted or corrupt catalogs; use `pogo_flash_file_check()` and reject those
statuses first when data must be preserved. Incompatible catalog versions are
refused. Replacement and defrag are not power-failure atomic.

Pogobios keeps its console policy and UART protocol in
[the BIOS adapter](../../pogobios/pffs/README.md). Its commands require an
explicit `format YES`. See [usage and persistence limits](../../readme.md#pffs-file-commands).

The private indexed-writer helpers support BIOS IR reception. Their caller must
use a checked begin operation, serialize all filesystem mutations until
finish/abort, and verify chunk completeness plus the content CRC before
publication. They do not add an indexed application API or alter v4 catalogs.
