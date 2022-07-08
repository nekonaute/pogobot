/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#define IR_MAGIC_REQ    "ir2PoG042ISIR"
#define FLASH_IS_OK     "FlashIsOK"

int ir_boot_loop();
uint8_t check_crc(struct sfl_frame*);
void print_frame(struct sfl_frame*);
