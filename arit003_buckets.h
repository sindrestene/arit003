//
// Created by sindre on 28.04.2020.

#ifndef _ARIT003_BUCKETS_H_
#define _ARIT003_BUCKETS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "arit003_struct.h"

struct arit003_bucket_struct {
    uint32_t zeroes_and_count[2];
};

struct arit003_bucket_struct* arit003_create_buckets(int num_buckets);

int arit003_free_buckets(struct arit003_bucket_struct* buckets);

int arit003_encode_and_update_bucket(struct arit003_bucket_struct *the_bucket, struct arit003_struct* enc_state, uint8_t val);

uint8_t arit003_decode_and_update_bucket(struct arit003_bucket_struct *the_bucket, struct arit003_struct* dec_state);

#ifdef __cplusplus
}
#endif

#endif //_ARIT003_BUCKETS_H_

