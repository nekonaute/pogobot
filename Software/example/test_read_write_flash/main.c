/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include "pogobot.h"

/* Erase 64kB in flash in RW authorized space */
/* Write and Read in each page (256 bytes)*/

int main(void) 
{

    pogobot_init();

    int cmp = 0;

    printf("init ok\n");

    /* One page */
    char vals_in[256];
    for(int i = 0; i<256; i++){
      vals_in[i] = i;
    }

    char vals_out[256];

    erase_write_section_flash();

    for(int k = 0; k<256; k++){
      pogobot_led_setColor(255,0,0);
      
      write_page_flash(0, vals_in);

      pogobot_led_setColor(0,255,0);
      
      read_page_flash(0, vals_out);

      pogobot_led_setColor(0,0,255);

      printf("Read page %d : ", k);
      for(int i = 0; i<256;i++){
        cmp += !(vals_out[i]==vals_in[i]);
      }
      if(cmp==0){
        printf("OK \n");
      }
      else{
        printf("NOK \n");  
      }

      erase_write_section_flash();
      read_page_flash(0, vals_out);
      printf("Read page %d after erasing : ", k);
      for(int i = 0; i<256;i++){
        cmp += !(vals_out[i]==255);
      }
      if(cmp==0){
        printf("OK \n");
      }
      else{
        printf("NOK \n");  
      }
    }

    return 0;
}
