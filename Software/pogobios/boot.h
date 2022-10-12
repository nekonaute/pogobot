/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#ifndef __BOOT_H
#define __BOOT_H

#define FLASH_OK_OFFSET 0x88000
#define USRPRG_CRC      0x80000
//#define SLF_CMD_CHECK   0x03

void __attribute__((noreturn)) boot(unsigned long r1, unsigned long r2, unsigned long r3, unsigned long addr);
int serialboot(void);
void update_led_status(void);
uint8_t check_flash_state(const char * data, uint32_t address_in_flash);
uint32_t get_uint32(unsigned char* data);
#endif /* __BOOT_H */
