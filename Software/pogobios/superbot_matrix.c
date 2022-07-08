/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#include "superbot_matrix.h"

#include <math.h> // for tanh

void matrix_multiply_vector(const int matrix_row_count, const int matrix_column_count, const NUMTYPE*matrix, NUMTYPE* const vector, NUMTYPE *out_vector)
{
    for (int row = 0; row < matrix_row_count; row++)
    {
        NUMTYPE *v = vector;
        NUMTYPE sum = 0;
        for (int column = 0; column < matrix_column_count; column++)
        {
            sum += *(matrix++) * *v;
        }
        *(out_vector++) = sum;
    }
}

void vector_apply_sigmoid_inplace(const int vector_count, NUMTYPE *vector)
{
        for (int i = 0; i < vector_count; i++)
        {
            NUMTYPE val = *vector;
            NUMTYPE newval = val; // tanhf(val);
            *(vector++) = newval;
        }
}
