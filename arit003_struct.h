//
// Created by sindre on 24.04.2020.

#ifndef _ARIT003_STRUCT_H_
#define _ARIT003_STRUCT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SUPPORT_RAW_BITS_V0003
#define SUPPORT_RAW_BITS_V0003 0
#endif

#if SUPPORT_RAW_BITS_V0003
// For debugging less than 64 bits can be stored in each bitBuffer.
#define RAW_BITS_BUFFER_SIZE_BITS_V0003 64
#endif

struct arit003_struct {
    // This is to ensure that the layout of x1 and x2 can be accessed as a lookup table
    // Note that the order must be x2 then x1, so that no extra enc_state->x[(bit_to_encode)] = mid;
	union {
		struct {
			uint64_t x2;
			uint64_t x1;
		};
		uint64_t x[2];
	};
    uint64_t mid;
    uint8_t *pushout_queue;
    uint64_t pushout_queue_count;
    uint64_t dec_loaded_state;
    uint64_t dec_pushout_queue_index;
    #if SUPPORT_RAW_BITS_V0003 == 1
    // When the first bit is pushed to the bitBuffer allocate a buffer 8 bytes ahead of current pushout_queue_count.
    //    If numFlushedBuffersOutstanding == 0 then set numFlushedBuffersOutstanding to 1.
    //    If numFlushedBuffersOutstanding > 0 then set numFlushedBuffersOutstanding+=1 and write to 8*numFlushedBuffersOutstanding.
    // When the 3'th arit0003 flush triggers write 2 bytes, skip 8*numFlushedBuffersOutstanding bytes, then write the last 1 byte.
    uint64_t bitBuffer; // The raw bits.
    int32_t countBitsUsed; // When this becomes 1 trigger an allocation. When this becomes 64 write to ptrStoreBufferInThePast, and set countBitsUsed=0.
    uint8_t *ptrStoreBufferInThePast; // This is where the bitBuffer will be written to.
    int32_t numFlushedBuffersOutstanding; // How many bitBuffer's has been created without a single arit0003 3 byte flush.
    int32_t countDownUntilStoreBuffer; // Do normal flush if positive, trigger skip(s) at 0, do normal flush if negative. Set to 2 a new buffer is created.
    int32_t buffersNextCountDown1; // This is the countDownUntilStoreBuffer when there are 2 buffers in the next 9 byte window.
    int32_t numFlushedBuffersOutstandingNext1;
    int32_t buffersNextCountDown2; // This is the countDownUntilStoreBuffer when there are 3 buffers in the next 9 byte window. This is the max also
    int32_t numFlushedBuffersOutstandingNext2;
    #endif
};

struct arit003_struct* arit003_struct_create(uint64_t pushout_queue_capacity, bool malloc_pushout_queue);

void arit003_struct_free(struct arit003_struct* stru, bool free_pushout_queue);

#ifdef __cplusplus
}
#endif

#endif //_ARIT003_STRUCT_H_
