
#include "pogobot.h"
#include "display.h"
#include "kalman.h"



//##############################################################
//##############################################################
// FONCTIONS DE LEDS

void set_all_leds(int r, int g, int b) {
  for (int led=0; led<=4; led++){
    pogobot_led_setColors(r, g, b, led);
  }
  return;
}

void anim_same(void) {
  for (int i=0; i<3; i++) {
    set_all_leds(255, 0, 0);
    msleep(300);
    set_all_leds(0, 255, 0);
    msleep(300);
    set_all_leds(0, 0, 255);
    msleep(300);
  }
  set_all_leds(255, 255, 255);
  msleep(800);
  set_all_leds(0, 0, 0);
  return;
}

void anim_blink(int r, int g, int b, int number_of_blinks) {
  for (int i=0; i<number_of_blinks; i++) {
    set_all_leds(r, g, b);
    msleep(250);
    set_all_leds(0, 0, 0);
    msleep(250);
  }
  return;
}


//##############################################################
//##############################################################
// FONCTIONS D'AFFICHAGE CONSOLE

void print_float(float i, int precision) {
  // prints a float to the console
  // precision is 10, 100, 1000... and represents the number of decimals to print
  int dec = (int)((i-(int)i) * precision);
  if (dec < 0) {
    dec = -1 * dec;
  }
  char buffer[10];
  if (i<0.0f && i>-1.0f) {
    sprintf(buffer, "-%d.%d", (int)i, dec);
  } else {
    sprintf(buffer, "%d.%d", (int)i, dec);
  }
  printf("%8s", buffer);
}

void print_f_list(float* list, int len, int precision) {
  for (int i=0; i<len; i++) {
    print_float(list[i], precision);
    printf("\t");
  }
}

void print_f_matrix(float mat[][6], int rows) {
  for (int i=0; i<C; i++) {
    printf("[");
    for (int j=0; j<rows; j++) {
      print_float(mat[i][j], 100);
      printf("  ");
    }
    printf("]\n");
  }
}