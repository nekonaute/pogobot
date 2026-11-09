/* Private platform selection for the canonical PFFS core.
 * Hardware is the default. Only host simulator builds opt into emulated erases;
 * compiling a normal SDK library never selects that behavior implicitly. */
#ifndef POGOBOT_PFFS_PLATFORM_H
#define POGOBOT_PFFS_PLATFORM_H
#ifdef POGOBOT_PFFS_SIMULATOR
#include <pogobase.h>
#else
#include <pogobot.h>
#include <spi.h>
#endif
#endif
