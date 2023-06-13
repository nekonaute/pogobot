
#include "pogobot.h"
#include "kalman.h"
#include "display.h"



//##############################################################
//##############################################################
// MATRIX OPERATIONS

void _multMatrixWidthC(float matRes[][C], float mat1[][C], float mat2[][C], int r1) {
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < C; j++) {
            matRes[i][j] = 0;
            for (int k = 0; k < C; k++) {
                matRes[i][j] += mat1[i][k] * mat2[k][j];
            }
        }
    }
}

void _addMatrixWidthC(float matRes[][C], float mat1[][C], float mat2[][C], int r) {
  for (int i=0; i<r; i++) {
    for (int j=0; j<C; j++) {
      matRes[i][j] = mat1[i][j] + mat2[i][j];
    }
  }
}

void _subtractMatrixWidthC(float matRes[][C], float mat1[][C], float mat2[][C], int r) {
  for (int i=0; i<r; i++) {
    for (int j=0; j<C; j++) {
      matRes[i][j] = mat1[i][j] - mat2[i][j];
    }
  }
}

void _transposeMatrixCbyC(float matRes[][C], float mat[][C]) {
  for (int i=0; i<C; i++)
    for (int j=0; j<C; j++)
      matRes[i][j] = mat[j][i];
}

void _identityMatrixCbyC(float matRes[][C]) {
  for (int i=0; i<C; i++) {
    for (int j=0; j<C; j++) {
      if (i==j) {
        matRes[i][j] = 1.0;
      } else {
        matRes[i][j] = 0.0;
      }
    }
  }
}

void _copyMatrixWidthC(float matRes[][C], float mat[][C], int r) {
  for (int i=0; i<r; i++)
    for (int j=0; j<C; j++)
      matRes[i][j] = mat[i][j];
}

void _pseudoInverseMatrixCbyC(float IM[][C], float mat[][C]) {
  // reproduced from the python code here:
  // https://integratedmlai.com/matrixinverse/
  float AM[C][C];
  float I[C][C];
  _copyMatrixWidthC(AM, mat, 6);
  _identityMatrixCbyC(I);
  _copyMatrixWidthC(IM, I, 6);

  for (int fd=0; fd<C; fd++) {
    float fdScaler = 1.0 / AM[fd][fd];
    int indices[C];
    for (int i=0; i<C; i++)
      indices[i] = i;
    for (int j=0; j<C; j++) {
      AM[fd][j] = AM[fd][j] * fdScaler;
      IM[fd][j] = IM[fd][j] * fdScaler;
    }
    for (int k=0; k<C; k++) {
      if (k != fd) {
        int i = indices[k];
        float crScaler = AM[i][fd];
        for (int j=0; j<C; j++) {
          AM[i][j] = AM[i][j] - crScaler * AM[fd][j];
          IM[i][j] = IM[i][j] - crScaler * IM[fd][j];
        }
      }
    }
  }
}







void combine_arrays(float res[], float arr1[], float arr2[], int len1, int len2) {
  int i;
  for (i=0; i<len1; i++) {
    res[i] = arr1[i];
  }
  for (int j=0; j<len2; j++) {
    res[i+j] = arr2[j];
  }
}

void split_array(float base[], float arr1[], float arr2[], int len1, int len2) {
  int i;
  for (i=0; i<len1; i++) {
    arr1[i] = base[i];
  }
  for (int j=0; j<len2; j++) {
    arr2[j] = base[i+j];
  }
}






//##############################################################
//##############################################################
// EXTENDED KALMAN FILTER
void initExtendedKalmanFilter(
    int power,
    float state_estimate_k_minus_1[][C],       // [1][6] 6x1
    float P_k_minus_1[][C],                    // [6][6] 6x6
    float A_k_minus_1[][C],                    // [6][6] 6x6
    float process_noise_v_k_minus_1[][C],      // [1][6] 6x1
    float Q_k[][C],                            // [6][6] 6x6
    float R_k[][C],                            // [6][6] 6x6
    float H_k[][C],                            // [6][6] 6x6
    float sensor_noise_w_k[][C]                // [1][6] 6x1
    ) {
      float initial_state_estimate[1][6] = {
        {0.0, 0.0, -GRAVITY, 0.0, 0.0, 0.0}};
      float initial_P_k[6][6] = {
        {0.1, 0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.1}
        };
      float initial_Q_k[6][6] = {
        {0.01, 0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.01, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.01, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.01, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.01, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.1}
        };
      float intial_process_noise[1][6] = {
        {0.1, 0.1, 0.1, 0.01, 0.01, 0.01}
      };
      float initial_sensor_noise[1][6] = {
        {0.07, 0.07, 0.07, 0.05, 0.05, 0.05}
      };
      _copyMatrixWidthC(state_estimate_k_minus_1, initial_state_estimate, 1);
      _copyMatrixWidthC(P_k_minus_1, initial_P_k, 6);
      _identityMatrixCbyC(A_k_minus_1);
      _copyMatrixWidthC(process_noise_v_k_minus_1, intial_process_noise, 1);
      _copyMatrixWidthC(Q_k, initial_Q_k, 6);
      if (power < 150) {
        float initial_R_k[6][6] = {
        {0.01, 0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.01, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.01, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.01, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.01, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.05}
        };
        _copyMatrixWidthC(R_k, initial_R_k, 6);
      } else if (power < 750) {
        float initial_R_k[6][6] = {
        {1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.2}
        };
        _copyMatrixWidthC(R_k, initial_R_k, 6);
      } else {
        float initial_R_k[6][6] = {
        {5.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 5.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 5.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 5.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 5.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.7}
        };
        _copyMatrixWidthC(R_k, initial_R_k, 6);
      }
      _identityMatrixCbyC(H_k);
      _copyMatrixWidthC(sensor_noise_w_k, initial_sensor_noise, 1);

    }

/*
# adapted from:
# Author: Addison Sears-Collins
# https://automaticaddison.com
# Description: Extended Kalman Filter example (two-wheeled mobile robot)
*/
void extendedKalmanFilter(
    float z_k_observation_vector[][C],         // [1][6] 6x1
    float state_estimate_k_minus_1[][C],       // [1][6] 6x1
    float P_k_minus_1[][C],                    // [6][6] 6x6
    float A_k_minus_1[][C],                    // [6][6] 6x6
    float process_noise_v_k_minus_1[][C],      // [1][6] 6x1
    float Q_k[][C],                            // [6][6] 6x6
    float R_k[][C],                            // [6][6] 6x6
    float H_k[][C],                            // [6][6] 6x6
    float sensor_noise_w_k[][C],               // [1][6] 6x1
    // returns:
    float state_estimate_k[][C],               // [1][6] 6x1
    float P_k[][C]                             // [6][6] 6x6
    ) {
      float tmpMat6x1[1][6];
      float tmpMat2_6x1[1][6];
      float tmpMat6x6[6][6];
      float tmpMat2_6x6[6][6];
      float tmpMat3_6x6[6][6];

      //######### Predict #########
      // Predict the state estimate
      _addMatrixWidthC(tmpMat6x1, state_estimate_k_minus_1, process_noise_v_k_minus_1, 1);
      _multMatrixWidthC(state_estimate_k, A_k_minus_1, tmpMat6x1, 1);
      // Predict the state covariance estimate
      _multMatrixWidthC(tmpMat6x6, A_k_minus_1, P_k_minus_1, 6);
      _transposeMatrixCbyC(tmpMat2_6x6, A_k_minus_1);
      _multMatrixWidthC(tmpMat3_6x6, tmpMat6x6, tmpMat2_6x6, 6);
      _addMatrixWidthC(P_k, tmpMat3_6x6, Q_k, 6);

      //######### Correct #########
      // Calculate the difference between the measurements and prediction
      float measurement_residual_y_k[1][6];
      _multMatrixWidthC(tmpMat6x1, H_k, state_estimate_k, 1);
      _addMatrixWidthC(tmpMat2_6x1, tmpMat6x1, sensor_noise_w_k, 1);
      _subtractMatrixWidthC(measurement_residual_y_k, z_k_observation_vector, tmpMat2_6x1, 1);
      // Calculate the measurement residual covariance
      float S_k[6][6];
      _multMatrixWidthC(tmpMat6x6, H_k, P_k, 6);
      _transposeMatrixCbyC(tmpMat2_6x6, H_k);
      _multMatrixWidthC(tmpMat3_6x6, tmpMat6x6, tmpMat2_6x6, 6);
      _addMatrixWidthC(S_k, tmpMat3_6x6, R_k, 6);         //S_k = H_k @ P_k @ H_k.T + R_k
      // Calculate the near-optimal Kalman gain
      float K_k[6][6];
      _multMatrixWidthC(tmpMat6x6, P_k, tmpMat2_6x6, 6);  //P_k @ H_k.T
      _pseudoInverseMatrixCbyC(tmpMat2_6x6, S_k);         //np.linalg.pinv(S_k)
      _multMatrixWidthC(K_k, tmpMat6x6, tmpMat2_6x6, 6);  //K_k = P_k @ H_k.T @ np.linalg.pinv(S_k)
      // Calculate an updated state estimate for time k
      _multMatrixWidthC(tmpMat2_6x1, K_k, measurement_residual_y_k, 1);
      _addMatrixWidthC(state_estimate_k, state_estimate_k, tmpMat2_6x1, 1);
      // Update the state covariance estimate for time k
      _multMatrixWidthC(tmpMat6x6, K_k, H_k, 6);
      _multMatrixWidthC(tmpMat2_6x6, tmpMat6x6, P_k, 6);
      _subtractMatrixWidthC(P_k, P_k, tmpMat2_6x6, 6);
    }





void pogobot_quick_calibrate(int power, int* leftMotorVal, int* rightMotorVal) {
  pogobot_calibrate(power, 500, 750, 15, 50.0f, leftMotorVal, rightMotorVal);
}

void pogobot_motor_jump_set(int power, int motor) {
  if (power > 512) {
    pogobot_motor_set(power, motor);
  } else {
    pogobot_motor_set(512, motor);
    msleep(50);
    pogobot_motor_set(power, motor);
  }
}


//#define RESULTS_SAVED 4
void pogobot_calibrate(int power, int startup_duration, int try_duration, int number_of_tries, float correction, int* leftMotorVal, int* rightMotorVal) {
    float acc[3];
    float gyro[3];

    // kalman arguments 
    float obs_vector_z_k[1][6];
    float state_estimate_k_minus_1[1][6];
    float P_k_minus_1[6][6];
    float A_k_minus_1[6][6];
    float process_noise_v_k_minus_1[1][6];
    float Q_k[6][6];
    float R_k[6][6];
    float H_k[6][6];
    float sensor_noise_w_k[1][6];

    // final results
    int powerLeft  = power;
    int powerRight = power;

    for (int i=0; i<number_of_tries; i++) {
      initExtendedKalmanFilter(
        power,
        state_estimate_k_minus_1,       // [1][6] 
        P_k_minus_1,                    // [6][6] 
        A_k_minus_1,                    // [6][6] 
        process_noise_v_k_minus_1,      // [1][6] 
        Q_k,                            // [6][6] 
        R_k,                            // [6][6] 
        H_k,                            // [6][6] 
        sensor_noise_w_k                // [1][6] 
      );
      // kalman results
      float state_estimate_k[1][6];
      float P_k[6][6];
      //float last_kalman_results[RESULTS_SAVED];
      //int result_index = 0;

      // STARTUP
      pogobot_motor_jump_set(motorL, powerLeft);
      pogobot_motor_jump_set(motorR, powerRight);
      msleep(startup_duration);
      printf("\nmotorLeft=%d ; motorRight=%d\n", powerLeft, powerRight);


      // MOVE, COLLECT DATA AND APPLY KALMAN
      time_reference_t timer;
      pogobot_stopwatch_reset(&timer);
      while (pogobot_stopwatch_get_elapsed_microseconds(&timer) / 1000 < try_duration) {
          pogobot_imu_read(acc, gyro);
          combine_arrays(*obs_vector_z_k, acc, gyro, 3, 3);
          extendedKalmanFilter(
              obs_vector_z_k,                 // [1][6]
              state_estimate_k_minus_1,       // [1][6]
              P_k_minus_1,                    // [6][6]
              A_k_minus_1,                    // [6][6] 
              process_noise_v_k_minus_1,      // [1][6] 
              Q_k,                            // [6][6] 
              R_k,                            // [6][6] 
              H_k,                            // [6][6] 
              sensor_noise_w_k,               // [1][6] 
              // returns:
              state_estimate_k,               // [1][6] 
              P_k                             // [6][6] 
          );
          _copyMatrixWidthC(P_k_minus_1, P_k, 6);
          _copyMatrixWidthC(state_estimate_k_minus_1, state_estimate_k, 1);
          // for debug the next line is reaaaally great
          //print_kalman(pogobot_stopwatch_get_elapsed_microseconds(&timer)/1000, state_estimate_k, acc, gyro);
      }

      // CORRECT MOTOR VALUES
      pogobot_motor_set(motorL, motorStop);
      pogobot_motor_set(motorR, motorStop);
      float gyro_z = state_estimate_k[0][5];
      //printf("\tgyroscope = ");
      //print_float(gyro_z, 1000);
      //printf("\n");
      int c = (int)(gyro_z * correction);
      if (powerLeft-c > 1023 || powerLeft-c < 0) {
        powerRight += c;
      } else {
        powerLeft -= c;
      }
      msleep(250);    // pour que le pogo revienne à l'arrêt
    }

    //printf("Calibration complete:\n\tLeft: %d\n\tRight: %d\n", powerLeft, powerRight);
    if (powerRight > 1023) powerRight = 1023;
    if (powerRight < 0) powerRight = 0;
    *leftMotorVal  = powerLeft;
    *rightMotorVal = powerRight;
    return;
}


void print_kalman(int i, float state_estimate_k[][6], float acc[], float gyro[3]) {
        float accNew[3];
        float gyroNew[3];
        split_array(*state_estimate_k, accNew, gyroNew, 3, 3);
        printf("----------------  Sans filtre[%d ms]  ----------------\n", i);
        printf("Acc = (");
        print_f_list(acc, 3, 100);
        printf(")\n");
        printf("Gyro = (");
        print_f_list(gyro, 3, 100);
        printf(")\n");

        printf("---------------- Avec filtre [%d ms]  ----------------\n", i);
        printf("Acc = (");
        print_f_list(accNew, 3, 100);
        printf(")\n");
        printf("Gyro = (");
        print_f_list(gyroNew, 3, 100);
        printf(")\n\n");
}



