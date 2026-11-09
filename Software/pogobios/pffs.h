#ifndef POGOBIOS_PFFS_H
#define POGOBIOS_PFFS_H
#include <stdbool.h>

/* Private console services; transfer state and controller installation stay
 * inside the BIOS while the shared file/log APIs are provided by pogolib. */
void bios_pffs_init(void);
void bios_pffs_service(void);
bool bios_pffs_busy(void);
bool bios_pffs_allows(const char *command);
bool bios_pffs_input_reset(void);
#endif
