//
// Created by sindre on 24.04.2020.

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "arit003_struct.h"

struct arit003_struct* arit003_struct_create(uint64_t pushout_queue_capacity, bool malloc_pushout_queue) {
    struct arit003_struct *stru;

    stru = (struct arit003_struct*) malloc(1 * sizeof(struct arit003_struct));
    memset(stru, 0, 1 * sizeof(struct arit003_struct));
    stru->x1 = 0;
    stru->x2 = UINT64_MAX; // actually, max+1 ... interacts with the odds calculation, fixed there
    stru->mid = 0x00001DECAFC0FFEEull;
    if(malloc_pushout_queue) {
        stru->pushout_queue = (uint8_t *) malloc(sizeof(uint8_t) * pushout_queue_capacity);
        memset(stru->pushout_queue, 0, pushout_queue_capacity * sizeof(uint8_t));
    }
    stru->pushout_queue_count = 0;
    stru->dec_loaded_state = 0x0000000000000000ull;
    stru->dec_pushout_queue_index = 0;

    #if SUPPORT_RAW_BITS_V0003 == 1
    stru->bitBuffer = 0;
    stru->countBitsUsed = 0;
    stru->ptrStoreBufferInThePast= 0;
    stru->numFlushedBuffersOutstanding = 0;
    stru->countDownUntilStoreBuffer = -1;
    stru->buffersNextCountDown1 = -1;
    stru->numFlushedBuffersOutstandingNext1 = 0;
    stru->buffersNextCountDown2 = -1;
    stru->numFlushedBuffersOutstandingNext2 = 0;
    #endif
    return stru;
}

void arit003_struct_free(struct arit003_struct* stru, bool free_pushout_queue){
    if(free_pushout_queue) {
        free(stru->pushout_queue);
    }
    free(stru);
}

