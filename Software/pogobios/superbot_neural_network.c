/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include <stdlib.h>
#include "superbot_matrix.h"
#include "superbot_neural_network.h"

// Layer 0 is input.

#define LAYER0_NEURON_COUNT 24
#define LAYER1_NEURON_COUNT 31
#define LAYER2_NEURON_COUNT 9

NUMTYPE weight_matrix_from_layer0_to_layer1 [ LAYER0_NEURON_COUNT * LAYER1_NEURON_COUNT ];
NUMTYPE weight_matrix_from_layer1_to_layer2 [ LAYER1_NEURON_COUNT * LAYER2_NEURON_COUNT ];

NUMTYPE layer0_activity[LAYER0_NEURON_COUNT];
NUMTYPE layer1_activity[LAYER1_NEURON_COUNT];
NUMTYPE layer2_activity[LAYER2_NEURON_COUNT];

void neural_network_matrix_populate( void )
{
    for (int i=0; i<LAYER0_NEURON_COUNT*LAYER1_NEURON_COUNT; i++)
    {
        weight_matrix_from_layer0_to_layer1[i]=rand();
    }
    for (int i=0; i<LAYER1_NEURON_COUNT*LAYER2_NEURON_COUNT; i++)
    {
        weight_matrix_from_layer1_to_layer2[i]=rand();
    }
}

void neural_network_input_populate( void )
{
    for (int i=0; i<LAYER0_NEURON_COUNT; i++)
    {
        layer0_activity[i]=rand();
    }
}

void compute_one_neural_network_step( void )
{
    matrix_multiply_vector(LAYER0_NEURON_COUNT, LAYER1_NEURON_COUNT, weight_matrix_from_layer0_to_layer1, layer0_activity, layer1_activity);
    vector_apply_sigmoid_inplace(LAYER1_NEURON_COUNT, layer1_activity);
    matrix_multiply_vector(LAYER1_NEURON_COUNT, LAYER2_NEURON_COUNT, weight_matrix_from_layer1_to_layer2, layer1_activity, layer2_activity);
    vector_apply_sigmoid_inplace(LAYER2_NEURON_COUNT, layer2_activity);
}
