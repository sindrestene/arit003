//
// Created by sindre on 23.04.2020.
// test code for arit003
// uses an aritmetic encoder to encode some bits into a buffer. then decodes and verifies.

#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#include "arit003.h"
#include "math.h"
#include "arit003_buckets.h"

// gcc -g -O3 -march=native arit003*.c -lm -o arit003_tests

#define MAX_BITS_TO_ENCODE 640000000ull
#define MAX_BYTES_IN_A_PUSHOUT_QUEUE (MAX_BITS_TO_ENCODE*2+100)

int test_encode_decode_with_timers(uint8_t* bits_to_encode, uint8_t* decoded_bits, uint16_t* odds_of_zero_uint16, int num_bits_to_encode) {
    printf2("running...\n");
    uint8_t *saved_pushout_queue = NULL; // the compressed buffer
    uint64_t saved_pushout_queue_count = 0; // length of the compressed buffer
    { // encode
        struct arit003_struct* enc_state = arit003_struct_create(MAX_BYTES_IN_A_PUSHOUT_QUEUE, true);
        clock_t time_before = clock();
        for (int bit_i = 0; bit_i < num_bits_to_encode; bit_i++) {
            arit003_encode_bit(enc_state, bits_to_encode[bit_i], odds_of_zero_uint16[bit_i]);
        }
        clock_t time_spent = clock() - time_before;
        printf2("ENC num_bits_to_encode=%d, encode_time_ms=%ld\n", num_bits_to_encode, (time_spent * 1000 / CLOCKS_PER_SEC));

        arit003_encode_finalize(enc_state);
        printf2("-----------------  decode forwards start, pushout_queue_count=%lu  ------------------------------------------------------------------\n", enc_state->pushout_queue_count);
        saved_pushout_queue = enc_state->pushout_queue;
        saved_pushout_queue_count = enc_state->pushout_queue_count;
        arit003_struct_free(enc_state, false);
    }
    { // decode
        struct arit003_struct* dec_state = arit003_struct_create(MAX_BYTES_IN_A_PUSHOUT_QUEUE, false);
        dec_state->pushout_queue = saved_pushout_queue;
        dec_state->pushout_queue_count = saved_pushout_queue_count;
        arit003_decode_init(dec_state);
        int num_bits_to_decode = num_bits_to_encode;
        clock_t time_before = clock();
        for (int bit_i = 0; bit_i < num_bits_to_decode; bit_i++) {
            uint8_t dec_bit_val_int = arit003_decode_bit(dec_state, odds_of_zero_uint16[bit_i]);
            decoded_bits[bit_i] = dec_bit_val_int;
        }
        clock_t time_spent = clock() - time_before;
        printf2("DEC num_bits_to_encode=%d, decode_time_ms=%ld\n", num_bits_to_encode, (time_spent * 1000 / CLOCKS_PER_SEC));

        printf2("Done with decode! pushout_queue_count=%ld, last_byte=0x%2.2X\n", dec_state->pushout_queue_count, dec_state->pushout_queue[dec_state->pushout_queue_count-1]);
        uint64_t real_size_bits = (dec_state->pushout_queue_count * 8ull);
        uint64_t real_size_bytes = dec_state->pushout_queue_count;
        double real_ratio = (double)real_size_bits / (double)num_bits_to_encode;
        double real_savings = 1.0 - real_ratio;
        printf2("num_bits_to_encode=%d, real_size_bits=%lu, real_size_bytes=%lu, real_ratio=%f, real_savings=%f\n", num_bits_to_encode, real_size_bits, real_size_bytes, real_ratio, real_savings);
        arit003_struct_free(dec_state, true);
    }
    //verify test
    for(int i = 0; i < num_bits_to_encode; i++) {
        if(bits_to_encode[i] != decoded_bits[i]) {
            printf2("ERROR, bits_to_encode[i] != decoded_bits[i], i=%d\n", i);
            exit(200);
        }
    }
    printf2("Verification: All correct!\n");
    return 0;
}

//run only a perf test with timers
int setup_test_encode_decode_with_timers() {
    printf2("running\n");
    uint8_t* bits_to_encode = (uint8_t*) malloc(sizeof(uint8_t) * MAX_BITS_TO_ENCODE);
    memset(bits_to_encode, 0, MAX_BITS_TO_ENCODE * sizeof(uint8_t));
    uint16_t *odds_of_zero_uint16 = (uint16_t*) malloc(sizeof(uint16_t) * MAX_BITS_TO_ENCODE);
    memset(odds_of_zero_uint16, 0, MAX_BITS_TO_ENCODE * sizeof(uint16_t));
    srand(42);//time(NULL));
    for(int i = 0; i < MAX_BITS_TO_ENCODE; i++) {
        bits_to_encode[i] = bits_to_encode[i] & (i % 2u) & (i % 3u) & (i % 5u);
        bits_to_encode[i] = 1 - bits_to_encode[i];
        odds_of_zero_uint16[i] = 4096;
    }
    uint8_t* decoded_bits = (uint8_t*) malloc(sizeof(uint8_t) * MAX_BITS_TO_ENCODE);
    memset(decoded_bits, 0, MAX_BITS_TO_ENCODE * sizeof(uint8_t));
    int num_bits_to_encode = MAX_BITS_TO_ENCODE;
    int rounds = 2;
    for(int i = 0; i < rounds; i++) {
        printf2("run i=%d out of rounds=%d\n", i, rounds);
        int err = test_encode_decode_with_timers(bits_to_encode, decoded_bits, odds_of_zero_uint16, num_bits_to_encode);
        if(err != 0) {
            printf2("test_encode_decode_with_timers returned err %d\n", err);
        }
    }
    free(bits_to_encode);
    free(decoded_bits);
    free(odds_of_zero_uint16);
    printf2("done\n");
    return 0;
}

int test_bucketing_overhead() {
    //encode some values, spread them by buckets .... use random.
    printf2("test_bucketing_overhead() running\n");
    srand(42);
    int num_values_to_store = MAX_BITS_TO_ENCODE;//1024*8000;
    num_values_to_store = 1024*8000;
    if(num_values_to_store > MAX_BITS_TO_ENCODE) {
        num_values_to_store = MAX_BITS_TO_ENCODE;
    }

    //malloc data to encode
    uint8_t* bits_to_encode = (uint8_t*) malloc(sizeof(uint8_t) * num_values_to_store);
    memset(bits_to_encode, 0, num_values_to_store * sizeof(uint8_t));
    uint32_t* bucket_ids_for_testing = (uint32_t*) malloc(sizeof(uint32_t) * num_values_to_store);
    memset(bucket_ids_for_testing, 0, num_values_to_store * sizeof(uint32_t));
    int num_buckets = 1024;
    num_buckets = 10;
    for(int i = 0; i < num_values_to_store; i++) {
        uint8_t bit = rand() % 2;
        int bucket_id = rand() % num_buckets;
        bits_to_encode[i] = bit;
        bucket_ids_for_testing[i] = bucket_id;
    }

    //set up structure for storing data (between encode and decode)
    uint8_t *saved_pushout_queue = NULL; // the compressed buffer
    uint64_t saved_pushout_queue_count = 0; // length of the compressed buffer
    int num_bytes_capacity_in_pushout_queue = num_values_to_store; //8 bits per value is typically overkill though, 16bit plus overhead is worst case
    //ENCODE
    {
        struct arit003_bucket_struct *buckets_for_encode = arit003_create_buckets(num_buckets);
        struct arit003_struct* enc_state = arit003_struct_create(num_bytes_capacity_in_pushout_queue, true);

        int num_bits_to_encode = num_values_to_store;
        //actually encode
        for (int bit_i = 0; bit_i < num_bits_to_encode; bit_i++) {
            int my_bucket_id = bucket_ids_for_testing[bit_i];
            uint8_t val = bits_to_encode[bit_i];
            int err = arit003_encode_and_update_bucket(&buckets_for_encode[my_bucket_id], enc_state, val);

            if(err != 0){
                printf2("err %d, bit_i=%d, my_bucket_id=%d, val=%d\n", err, bit_i, my_bucket_id, val);
                exit(err);
            }
        }
        //printf2("encode end, do finalize (put remainder as byte into the out buffer)\n");
        arit003_encode_finalize(enc_state);

        //save things for decode
        saved_pushout_queue = enc_state->pushout_queue;
        saved_pushout_queue_count = enc_state->pushout_queue_count;
        arit003_struct_free(enc_state, false);
        arit003_free_buckets(buckets_for_encode);
    }
    printf2("-----------------  encode done, saved_pushout_queue_count=%lu  ------------------------------------------------------------------\n", saved_pushout_queue_count);
    uint8_t* decoded_bits = (uint8_t*) malloc(sizeof(uint8_t) * num_values_to_store);
    memset(decoded_bits, 0, num_values_to_store * sizeof(uint8_t));
    //DECODE
    {
        struct arit003_bucket_struct *buckets_for_decode = arit003_create_buckets(num_buckets);
        struct arit003_struct* dec_state = arit003_struct_create(num_bytes_capacity_in_pushout_queue, false);
        dec_state->pushout_queue = saved_pushout_queue; //should end with zeroes, enables faster loading
        dec_state->pushout_queue_count = saved_pushout_queue_count; //not used
        arit003_decode_init(dec_state);

        int num_bits_to_decode = num_values_to_store;
        for (int bit_i = 0; bit_i < num_bits_to_decode; bit_i++) {
            int my_bucket_id = bucket_ids_for_testing[bit_i];

            uint8_t val = arit003_decode_and_update_bucket(&buckets_for_decode[my_bucket_id], dec_state);
            decoded_bits[bit_i] = val;

            if (val != bits_to_encode[bit_i]) {
                printf2("ERROR, dec_bit_val_int != bits_to_encode[bit_i], bit_i=%d\n", bit_i);
                exit(203);
            }
        }
        printf2("Done with decode! pushout_queue_count=%ld, last_byte=0x%2.2X\n", dec_state->pushout_queue_count, dec_state->pushout_queue[dec_state->pushout_queue_count-1]);

        uint64_t real_size_bits = (dec_state->pushout_queue_count * 8ull);
        uint64_t real_size_bytes = dec_state->pushout_queue_count;
        double real_ratio = (double)real_size_bits / (double)num_bits_to_decode;
        double real_savings = 1.0 - real_ratio;
        printf2("num_bits_to_encode=%d, real_size_bits=%lu, real_size_bytes=%lu, real_ratio=%f, real_savings=%f\n", num_bits_to_decode, real_size_bits, real_size_bytes, real_ratio, real_savings);
        arit003_struct_free(dec_state, true);
        arit003_free_buckets(buckets_for_decode);
    }

    //verify test (again)
    for(int i = 0; i < num_values_to_store; i++) {
        if(bits_to_encode[i] != decoded_bits[i]) {
            printf2("ERROR, bits_to_encode[i] != decoded_bits[i], i=%d\n", i);
            exit(202);
        }
    }
    printf2("Verification: All correct!\n");
    free(bits_to_encode);
    free(decoded_bits);
    free(bucket_ids_for_testing);
    return 0;
}

int main() {
    test_bucketing_overhead();
    setup_test_encode_decode_with_timers();

    return 0;
}
