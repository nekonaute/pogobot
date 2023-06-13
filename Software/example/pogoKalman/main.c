
#include "pogobot.h"
#include "kalman.h"
#include "display.h"

int main(int argc, char* argv[]) {
    pogobot_init();
    printf("init ok\n");
    anim_same();

    int leftMotorVal;
    int rightMotorVal;

    pogobot_quick_calibrate(700, &leftMotorVal, &rightMotorVal);
    printf("Calibration ok : powerLeft = %d ; powerRight = %d\n", leftMotorVal, rightMotorVal);

    pogobot_motor_jump_set(motorL, leftMotorVal);
    pogobot_motor_jump_set(motorR, rightMotorVal); 

    anim_same();
    
    return 1;
}