/**
 * POGOBOT
 *
 * Copyright © 2022 Sorbonne Université ISIR
 * This file is licensed under the Expat License, sometimes known as the MIT License.
 * Please refer to file LICENCE for details.
**/

#ifdef POGOBOT_VERSION
#define ADC_INPUTS 8
#else
#define ADC_INPUTS 4
#endif

uint32_t ADC_Read(uint8_t channel);
