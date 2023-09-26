/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
 **/

#include "pogobot.h"
#include "math.h"

#define _EXP2(a, b) a ## b
#define _EXP(a, b) _EXP2(a ## e,b)
#define _CONSTPOW(C, x) ((int)_EXP(C, x))
#define _print_float_2(N, P) printf("%d.%d", (int)N, (int)((N - (int)N)*(_CONSTPOW(10, P)/10)))
#define _print_float_1(N) _print_float_2(N, 3)
#define _print_float_X(x,A,B,FUNC,...) FUNC
#define print_float(...) _print_float_X(,##__VA_ARGS__,_print_float_2(__VA_ARGS__),_print_float_1(__VA_ARGS__))


double val;
float fval;

int main (void) {

    pogobot_init();
    srand( pogobot_helper_getRandSeed() );
    printf("init ok\n");
    
    double const foobar = 4.2;
    printf("Can compile 1: ");
    print_float(foobar);
    printf("\n");

    float const ffoobar = 4.2f;
    printf("Can compile 2: ");
    print_float(foobar, 5);
    printf("\n");

    val = exp(foobar);
    printf("Can Compile 3: %d.%d\n", (int)val, (int)((val - (int)val)*1000));
    printf("Can Compile 4: %f\n", val); // shouldn't print
    // print float with 3 digits
    printf("print float "); print_float(val, 3); printf("\n");

    fval = expf(ffoobar);
    printf("Can compile 5: %d.%d\n", (int)fval, (int)((fval - (int)fval)*1000));

}

