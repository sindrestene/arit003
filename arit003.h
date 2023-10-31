//
// Created by sindre on 23.04.2020.

#ifndef _ARIT003_H_
#define _ARIT003_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "arit003_struct.h"

#define __SHORT_FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define printf2(fmt, args...) fprintf(stdout, "DEBUG: %s::%s():%4d: " fmt, __SHORT_FILENAME__, __FUNCTION__, __LINE__, ##args)

uint16_t fix_odds_as_int_16bit(float odds_as_float);
int arit003_encode_bit(struct arit003_struct *enc_state, uint8_t bit_to_encode, uint16_t odds_of_zero_uint16_t);
int arit003_encode_finalize(struct arit003_struct *enc_state);
int arit003_decode_init(struct arit003_struct *dec_state);
uint8_t arit003_decode_bit(struct arit003_struct *dec_state, uint16_t odds_of_zero_uint16_t);

#ifdef __cplusplus
}
#endif

#endif //_ARIT003_H_

