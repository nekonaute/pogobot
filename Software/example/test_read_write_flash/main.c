/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include "pogobot.h"

int main(void)
{
    /* Test the old 8-bit limit, the 1 MiB boundary, and the final v3 page. */
    const uint16_t pages[] = {0, 255, 256, 1791, 1792,
                              POGOBOT_USER_FLASH_PAGE_COUNT - 1};
    char written[POGOBOT_USER_FLASH_PAGE_SIZE];
    char read_back[POGOBOT_USER_FLASH_PAGE_SIZE];
    int errors = 0;

    pogobot_init();
    printf("Erasing %u KiB user flash section\n",
           (unsigned int)(POGOBOT_USER_FLASH_PAGE_COUNT *
                          POGOBOT_USER_FLASH_PAGE_SIZE / 1024u));
    erase_write_section_flash();

    for (unsigned int p = 0; p < sizeof(pages) / sizeof(pages[0]); p++) {
        uint16_t page = pages[p];
        unsigned int not_erased = 0;

        /* Stop before programming if a page is outside the erased region. */
        read_page_flash(page, read_back);
        for (unsigned int i = 0; i < sizeof(read_back); i++)
            if ((uint8_t)read_back[i] != 0xffu)
                not_erased++;
        if (not_erased) {
            printf("Page %u: %u bytes not erased; stopping\n",
                   (unsigned int)page, not_erased);
            return 1;
        }

        for (unsigned int i = 0; i < sizeof(written); i++)
            written[i] = (char)(page + i);
        write_page_flash(page, written);
        read_page_flash(page, read_back);
        if (memcmp(written, read_back, sizeof(written)) != 0) {
            errors++;
            printf("Page %u: write/read mismatch\n", (unsigned int)page);
        } else {
            printf("Page %u: OK\n", (unsigned int)page);
        }
    }

    printf("Flash page test: %s (%d errors)\n", errors ? "FAIL" : "PASS", errors);
    return errors ? 1 : 0;
}
