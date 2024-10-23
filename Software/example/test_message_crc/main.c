/**
 * POGOBOT
 *
 * Copyright © 2023 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include "pogobot.h"
#include "time.h"

#define CODENAME "TEST CORRUPTED MESSAGES"
#define INFRARED_POWER 2 // 1,2,3
#define FQCY 20 // control update frequency. 30Hz | 60 Hz | 90 Hz | etc.
#define MAX_NB_OF_MSG 3 // max. number of messages per step which this robot can record // 3


// ********************************************************************************
// * Initialization: Pogobot message structure
// ********************************************************************************

typedef struct RawMessage {
    uint8_t m1a;
    uint8_t m2a;
    uint8_t m3a;
    uint8_t m4a;
    uint8_t m5a;
    uint8_t m6a;
    uint8_t m1b;
    uint8_t m2b;
    uint8_t m3b;
    uint8_t m4b;
    uint8_t m5b;
    uint8_t m6b;
} RawMessage;

#define MSG_SIZE sizeof(RawMessage) // number of bytes

typedef union message_template {
    uint8_t msg_array[MSG_SIZE];
    RawMessage msg_values;
} message;


// ********************************************************************************
// * main
// ********************************************************************************

int main(void) {

    // init (mandatory)
    pogobot_init();
    srand(pogobot_helper_getRandSeed()); // initialize the random number generator
    pogobot_infrared_set_power(INFRARED_POWER); // set the power level used to send all the next messages

    // ********************************************************************************
    // * Initialization: hanabi experimental parameters and initial conditions
    // ********************************************************************************

    time_reference_t mystopwatch; // timer for step synchronization 
    uint32_t microseconds = 0; // counter for step synchronization

    uint8_t led1_status = 0;

    uint16_t den_p_send_per_step = 1; // probability to send a message per step (1/den_p_send_per_step)
    uint16_t nb_msg_sent_all = 0; // total number of unique messages sent
    uint16_t counter_rcvd_msgs = 0;
    uint8_t data[MSG_SIZE]; // message to send, containing uint8_t data

    message msg_from_neighbor;
    for (uint16_t i=0; i != MSG_SIZE; i++) // initialization
        msg_from_neighbor.msg_array[i] = 0;

    message my_msg;
    for (uint16_t i=0; i != MSG_SIZE; i++) // initialization
        my_msg.msg_array[i] = 0;
        

    // ********************************************************************************
    // * Main loop
    // ********************************************************************************

    while (1)
    {
        pogobot_stopwatch_reset(&mystopwatch); // reset of the timer, for step synchronization

        // ********************************************************************************
        // * Get messages (if any)
        // ********************************************************************************

        pogobot_infrared_update(); // infrared checks for received data. Then, messages are decoded and insered in a FIFO.

        if (pogobot_infrared_message_available()) { // read FIFO buffer - any message(s)?
            counter_rcvd_msgs = 0;

            if (!led1_status) { // 0: off; 1: on
                led1_status = 1;
                pogobot_led_setColors(0, 0, 25, 1); // led blue: received msg notification from any direction
            }

            // Record one message per robot w/ limitations on #robots and #messages (first come, first serve)
            while (pogobot_infrared_message_available() && counter_rcvd_msgs < MAX_NB_OF_MSG) {                
                // Recover the next message inside the message queue and stock it in the "mr" message_t structure, then in the "msg_from_neighbor" structure.  
                message_t mr;
                pogobot_infrared_recover_next_message(&mr);

                // Elaborate robot messages only (avoid controllers messages). NB: this condition works only with long headers
                if (mr.header._packet_type != ir_t_short) {
                    printf("[RECV] This message is discarded because it doesn't come from a Pogobot\n");
                    continue;
                }
       
                // Read the received message and stock it in the msg_from_neighbor structure
                for (uint16_t i = 0; i != MSG_SIZE; i++)
                    msg_from_neighbor.msg_array[i] = mr.payload[i];

                // DEBUG RECEPTION
                for ( uint16_t i = 0; i != MSG_SIZE/2; i++ ) {
                    if (msg_from_neighbor.msg_array[i] != msg_from_neighbor.msg_array[i+(MSG_SIZE/2)]) {
                        for (uint16_t i = 0; i < 5; i++)
                            pogobot_led_setColors(255, 0, 0, i); // red
                        while(1) {
                            printf("DEBUG RECEPTION\nm1: %d, %d \nm2: %d, %d \nm3: %d, %d \nm4: %d, %d \nm5: %d, %d \nm6: %d, %d \n###\n", msg_from_neighbor.msg_values.m1a, msg_from_neighbor.msg_values.m1b, msg_from_neighbor.msg_values.m2a, msg_from_neighbor.msg_values.m2b, msg_from_neighbor.msg_values.m3a, msg_from_neighbor.msg_values.m3b, msg_from_neighbor.msg_values.m4a, msg_from_neighbor.msg_values.m4b, msg_from_neighbor.msg_values.m5a, msg_from_neighbor.msg_values.m5b, msg_from_neighbor.msg_values.m6a, msg_from_neighbor.msg_values.m6b);
                            sleep(1);
                        }
                    }
                }
                counter_rcvd_msgs++;
            }
        }
        else {
            led1_status = 0;
            pogobot_led_setColors(0, 0, 0, 1); // led 1 off
        }
        pogobot_infrared_clear_message_queue();


        // ********************************************************************************
        // * Send message
        // ********************************************************************************

        if (rand()%den_p_send_per_step <= 1) {

            // Composing a new message to send
            my_msg.msg_values.m1a = 170; // exa AA
            my_msg.msg_values.m2a = 187; // exa BB
            my_msg.msg_values.m3a = 204; // exa CC
            my_msg.msg_values.m4a = 221; // exa DD
            my_msg.msg_values.m5a = 238; // exa EE
            my_msg.msg_values.m6a = 255; // exa FF
            my_msg.msg_values.m1b = 170;
            my_msg.msg_values.m2b = 187;
            my_msg.msg_values.m3b = 204;
            my_msg.msg_values.m4b = 221;
            my_msg.msg_values.m5b = 238;
            my_msg.msg_values.m6b = 255;

            // Convert the message into an uint8_t pointer
            for ( uint16_t i = 0; i != MSG_SIZE; i++ )
                data[i] = my_msg.msg_array[i];

            // DEBUG BUG SEND 1
            for ( uint16_t i = 0; i != MSG_SIZE; i++ ) {
                if (data[i] != my_msg.msg_array[i]) {
                    for (uint16_t i = 0; i < 5; i++)
                        pogobot_led_setColors(255, 255, 255, i); // white
                    while(1) {
                        printf("DEBUG SEND1\nm1: %d, %d \nm2: %d, %d \nm3: %d, %d \nm4: %d, %d \nm5: %d, %d \nm6: %d, %d \n###\n", my_msg.msg_values.m1a, my_msg.msg_values.m1b, my_msg.msg_values.m2a, my_msg.msg_values.m2b, my_msg.msg_values.m3a, my_msg.msg_values.m3b, my_msg.msg_values.m4a, my_msg.msg_values.m4b, my_msg.msg_values.m5a, my_msg.msg_values.m5b, my_msg.msg_values.m6a, my_msg.msg_values.m6b);
                        sleep(1);
                    }
                }
            }

            // DEBUG BUG SEND 2
            for ( uint16_t i = 0; i != MSG_SIZE/2; i++ ) {
                if (my_msg.msg_array[i] != my_msg.msg_array[i+(MSG_SIZE/2)]) {
                    for (uint16_t i = 0; i < 5; i++)
                        pogobot_led_setColors(0, 0, 255, i); // blue
                    while(1) {
                        printf("DEBUG SEND2\nm1: %d, %d \nm2: %d, %d \nm3: %d, %d \nm4: %d, %d \nm5: %d, %d \nm6: %d, %d \n###\n", my_msg.msg_values.m1a, my_msg.msg_values.m1b, my_msg.msg_values.m2a, my_msg.msg_values.m2b, my_msg.msg_values.m3a, my_msg.msg_values.m3b, my_msg.msg_values.m4a, my_msg.msg_values.m4b, my_msg.msg_values.m5a, my_msg.msg_values.m5b, my_msg.msg_values.m6a, my_msg.msg_values.m6b);
                        sleep(1);
                    }
                }
            }
            pogobot_infrared_sendShortMessage_omni((uint8_t *)(data), MSG_SIZE);
            nb_msg_sent_all = nb_msg_sent_all + 1;
        }


        // ********************************************************************************
        // * Step synchronize: wait for next step (if not timed out already)
        // ********************************************************************************

        microseconds = pogobot_stopwatch_get_elapsed_microseconds(&mystopwatch);

        if (microseconds < (1000000 / FQCY)) { // 1 s = 1000000 microseconds
            pogobot_led_setColors(0, 25, 0, 4);
            msleep(((1000000 / FQCY) - microseconds) / 1000); // wait for next step. NB: msleep in ms
        }
        else {
            printf("[TIME] Step took %lu usec, should be less than %u usec. [ ### TIME OVERFLOW ### ]\n", microseconds, (1000000 / FQCY));
            pogobot_led_setColors(25, 25, 25, 4); // too slow. Continue directly to next step
        }
    }
}
