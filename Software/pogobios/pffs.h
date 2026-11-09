#ifndef POGOBIOS_PFFS_H
#define POGOBIOS_PFFS_H
#include <stdbool.h>

/* Private BIOS services: no PFFS state or APIs are added to libpogobot.a. */
void bios_pffs_init(void);
void bios_pffs_service(void);
bool bios_pffs_busy(void);
bool bios_pffs_allows(const char *command);
bool bios_pffs_input_reset(void);
#endif
