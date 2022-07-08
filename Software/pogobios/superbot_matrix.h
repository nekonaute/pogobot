/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/


#ifndef __SUPERBOT_MATRIX__H__
#define __SUPERBOT_MATRIX__H__

#include <stdint.h>

#define NUMTYPE uint16_t

/* typedef sb_vector ## NUMTYPE */
/* { */
/*     int length; */
/*     NUMTYPE *c; */
/* } */

/* typedef sb_matrix ## NUMTYPE */
/* { */
/*     int rowcount; */
/*     int columncount; */
/*     NUMTYPE *c; */
/* } */

void matrix_multiply_vector(const int matrix_row_count, const int matrix_column_count, const NUMTYPE* matrix, NUMTYPE* const vector, NUMTYPE *out_vector);

void vector_apply_sigmoid_inplace(const int vector_count, NUMTYPE *vector);

#endif /* __SUPERBOT_MATRIX__H__ */

