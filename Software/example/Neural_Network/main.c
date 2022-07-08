/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#include "pogobot.h"
#include "math.h"

#define debug_mode  2 // 1: Weight =1, 2: rand simple, 0: rand float /not functionnal 
#define debug_actFn  1 // 0: Input = output, 1: tangh
#define display  0 // 0: no display, 1: display

// constraint: all parameter values must be >0
// Warning : it is working because nb_hidden_nodes_per_layer > nb_input and nb_output
#define nb_inputs  20
#define nb_outputs 10
#define nb_hidden_layers 3
#define nb_hidden_nodes_per_layer 30

float getRandomWeight( void ); 
float getRandomInput( void );
float apply_activation_function ( float value);


float getRandomWeight( void ) {
  if (debug_mode == 1)
  {
    return 1.0;
  } else if (debug_mode == 2){
    return (rand()%2)-1;
  } else {
    return (rand()/(1.0*RAND_MAX))*2.0 -1;
  }
}

float getRandomInput( void ) {
  if (debug_mode == 1)
  {
    return 1.0;
  } else if (debug_mode == 2){
    return (rand()%2);
  } else {
    return (rand()/(1.0*RAND_MAX));
  }
}

float apply_activation_function ( float value) {
  if (debug_actFn == 0){
    return value;
  } else {
    return tanhf(value); // hyperbolic tangeante
  }
}

static void main_compute( void )
{

    float inputs[nb_inputs];
    for (int i = 0; i < nb_inputs; i++)
    {
        inputs[i] = getRandomInput();    //inputs 0 1    // weight -1 1
    }
    

    float outputs[nb_outputs] = {0}; 

    // Warning : it is working because nb_hidden_nodes_per_layer > nb_output
    float weights[nb_hidden_layers+1][nb_hidden_nodes_per_layer * nb_hidden_nodes_per_layer];

    for ( int i = 0; i < nb_hidden_layers+1; i++)
    {   
        for (int k = 0; k < nb_hidden_nodes_per_layer * nb_hidden_nodes_per_layer; k++)
        {
            weights[i][k] = getRandomWeight();
        }
    }
    

    int iteration = 0;
    int max_iterations = 100;

    int dimensions = nb_inputs * nb_hidden_nodes_per_layer 
                    + ( nb_hidden_layers - 1 ) * ( nb_hidden_nodes_per_layer * nb_hidden_nodes_per_layer) 
                    + nb_hidden_nodes_per_layer * nb_outputs;
    printf ("dimensions: %d\n", dimensions);

    time_reference_t mystopwatch;
    pogobot_stopwatch_reset( &mystopwatch );
   

    while (iteration < max_iterations)
    {

        if (iteration % 10 == 0) {
            printf (" iteration no. %d/%d\n",iteration+1,max_iterations);
        }

        if (display){
            printf("inputs : \n");
            for (int i = 0; i < nb_inputs; i++)
            {
                printf("%.2f ", inputs[i]);
            }
            printf("\n");
            
            printf ("weights: \n");
            for (int i = 0; i < nb_hidden_layers+1; i++)
            {
                printf("%.2f ", weights[i][0]);
                printf("%.2f ", weights[i][1]);
                printf("%.2f ", weights[i][2]);
                printf("; ");
            }
            printf ("\n");
            
        }


        float current_layer[nb_hidden_nodes_per_layer];
        int current_layer_size = nb_inputs;

        for (int x = 0; x < current_layer_size; x++)
        {
            current_layer[x] = inputs[x];
        }
        
        
        int index_layer = 0;
        int index_weight = 0;
        int index_next_layer = 0;

        float next_layer[nb_hidden_nodes_per_layer]={0};


        for (int i = 0; i < nb_hidden_layers; i++)
        {
            
            index_weight = 0;
            index_next_layer = 0;

            for (int j = 0; j < nb_hidden_nodes_per_layer; j++)
            {
                float value = 0.0;
                for (int k = 0; k < current_layer_size; k++)
                {
                    value = value + current_layer[k] * weights[index_layer][index_weight];
                    index_weight ++;
                }
                next_layer[index_next_layer] = apply_activation_function(value);
                index_next_layer++;
            }

            current_layer_size = nb_hidden_nodes_per_layer;
            for (int x = 0; x < current_layer_size; x++)
            {
                current_layer[x] = next_layer[x];
            }
            index_layer++;

            if (display) {
                printf("layer no. %d : ", index_layer);
                for (int x = 0; x < current_layer_size; x++)
                {
                    printf("%.2f ", current_layer[x]);
                }
                printf ("\n");
            }
            
        }
        
        index_weight = 0;
        for (int j = 0; j < nb_outputs; j++)
        {
            float value = 0.0;
            for (int k = 0; k < current_layer_size; k++)
            {
                value = value + current_layer[k] * weights[index_layer][index_weight];
                index_weight ++;
            }
            outputs[j] = apply_activation_function(value);
            
        }

        if (display)
        {
            printf("outputs : ");
            for (int x = 0; x < nb_outputs; x++)
            {
                printf("%f ", outputs[x]);
            }
            printf("\n");
        }

        for ( int i = 0; i < nb_hidden_layers+1; i++)
        {   
            for (int k = 0; k < nb_hidden_nodes_per_layer * nb_hidden_nodes_per_layer; k++)
            {
                weights[i][k] = getRandomWeight();
            }
        }

        iteration = iteration + 1;

    }        

    uint32_t microseconds = pogobot_stopwatch_get_elapsed_microseconds( &mystopwatch );
    printf( "Duration: %lu microseconds", microseconds );

}

int main(void) {

    pogobot_init();
    pogobot_led_setColor( 0, 0, 0 );
    printf("init ok\n");

    /* initialize random */
    srand( pogobot_helper_getRandSeed() );

    main_compute();

}
