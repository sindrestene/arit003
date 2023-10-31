//
// Created by sindre on 28.04.2020.

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "arit003_buckets.h"
#include "arit003.h"


#define ARIT003_BUCKET_INIT_NUM_ZEROES 1
#define ARIT003_BUCKET_INIT_COUNT (ARIT003_BUCKET_INIT_NUM_ZEROES+1)
#define ARIT003_BUCKET_COUNT_CHANGE_AMT 1

struct arit003_bucket_struct* arit003_create_buckets(int num_buckets) {
    struct arit003_bucket_struct *buckets_for_encode;
    buckets_for_encode = (struct arit003_bucket_struct*) malloc(num_buckets * sizeof(struct arit003_bucket_struct));
    for(int i = 0 ; i < num_buckets; i++) {
        buckets_for_encode[i].zeroes_and_count[0] = ARIT003_BUCKET_INIT_NUM_ZEROES;
        buckets_for_encode[i].zeroes_and_count[1] = ARIT003_BUCKET_INIT_COUNT;
    }
    return buckets_for_encode;
}

int arit003_free_buckets(struct arit003_bucket_struct* buckets) {
    free(buckets);
    return 0;
}

#define ARIT003_DEBUG_BUCKETS 0
int arit003_encode_and_update_bucket(struct arit003_bucket_struct *the_bucket, struct arit003_struct* enc_state, uint8_t val) {
    //TODO odds calculation at END of bucket
    uint32_t zeroes_up = the_bucket->zeroes_and_count[0] << 16u;
    uint16_t odds_of_zero_uint16_t = zeroes_up / the_bucket->zeroes_and_count[1]; //minimum answer is 1, if [0] is 1 and [1] is 65535
    //TODO try with integer mult instead of div
    int err = arit003_encode_bit(enc_state, val, odds_of_zero_uint16_t);
#if ARIT003_DEBUG_BUCKETS == 1
    if (err != 0) {
        printf2("bucket zeroes=%4d, bucket count=%4d, odds_of_zero=%4d\n", the_bucket->zeroes_and_count[0], the_bucket->zeroes_and_count[1], odds_of_zero_uint16_t);
        return err;
    }
#endif
    //printf2("pre; zeroes=0x%8.8X, count=0x%8.8X\n", the_bucket->zeroes_and_count[0], the_bucket->zeroes_and_count[1]);
    the_bucket->zeroes_and_count[0] += (val == 0 ? ARIT003_BUCKET_COUNT_CHANGE_AMT : 0);
    the_bucket->zeroes_and_count[1] += ARIT003_BUCKET_COUNT_CHANGE_AMT;
    //printf2("mid; zeroes=0x%8.8X, count=0x%8.8X\n", the_bucket->zeroes_and_count[0], the_bucket->zeroes_and_count[1]);

    //reduce both zeroes and count if the count is large
    uint8_t shift_or_not = the_bucket->zeroes_and_count[1] >= (65536 - ARIT003_BUCKET_COUNT_CHANGE_AMT);

    the_bucket->zeroes_and_count[0] >>= shift_or_not;
    the_bucket->zeroes_and_count[0] |= (shift_or_not & 0x0001u); //hacky minimum 1
    the_bucket->zeroes_and_count[1] >>= shift_or_not;
    the_bucket->zeroes_and_count[1] |= (shift_or_not ? 0x0003u : 0); //hacky minimum 3

    //printf2("end; zeroes=0x%8.8X, count=0x%8.8X\n", the_bucket->zeroes_and_count[0], the_bucket->zeroes_and_count[1]);
    return err;
}

uint8_t arit003_decode_and_update_bucket(struct arit003_bucket_struct *the_bucket, struct arit003_struct* dec_state) {

    uint32_t zeroes_up = the_bucket->zeroes_and_count[0] << 16u;
    uint16_t odds_of_zero_uint16_t = zeroes_up / the_bucket->zeroes_and_count[1]; //minimum answer is 1, if [0] is 1 and [1] is 65535

    uint8_t val = arit003_decode_bit(dec_state, odds_of_zero_uint16_t);
    the_bucket->zeroes_and_count[0] += (val == 0 ? ARIT003_BUCKET_COUNT_CHANGE_AMT : 0);
    the_bucket->zeroes_and_count[1] += ARIT003_BUCKET_COUNT_CHANGE_AMT;

    //reduce both zeroes and count if the count is large
    uint8_t shift_or_not = the_bucket->zeroes_and_count[1] >= (65536 - ARIT003_BUCKET_COUNT_CHANGE_AMT);

    the_bucket->zeroes_and_count[0] >>= shift_or_not;
    the_bucket->zeroes_and_count[0] |= (shift_or_not & 0x0001u); // minimum 1
    the_bucket->zeroes_and_count[1] >>= shift_or_not;
    the_bucket->zeroes_and_count[1] |= (shift_or_not ? 0x0003u : 0); // minimum 3

    return val;
}
