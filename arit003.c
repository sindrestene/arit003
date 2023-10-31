//
// Created by sindre on 23.04.2020.

#include "arit003.h"

#define UINT16_MAX_DIV (1.0 / (double) (1ul << 16ul))

// 32bit: arbitrary tradeoff between "loss of range due to pushout" and "less accuracy due to too few bits in remaining range". tested 16/24/32 and 32 performed best.
#define ARIT003_INTERNAL_PUSH_N_BITS 32

inline uint16_t fix_odds_as_int_16bit(float odds_as_float) {
    uint16_t odds_fix_int = (odds_as_float * 65535);
    odds_fix_int++;
    #if ARIT003_INTERNAL_PUSH_N_BITS == 32
    if((odds_fix_int>>8) == 0) odds_fix_int = 1<<8;
    #endif
    return odds_fix_int;
}

#define ARIT003_USE_SLOW_ERROR_CHECKS 0
#if ARIT003_INTERNAL_PUSH_N_BITS == 16
#define ARIT003_INTERNAL_MIN_RANGE 0x000400000000ull
#define ARIT003_INTERNAL_SHIFT_AMT 48ull
#define ARIT003_INTERNAL_SHIFT_MASK 0x0000FFFFFFFFFFFFull
#elif ARIT003_INTERNAL_PUSH_N_BITS == 24
//smaller range is better for synthetic data set (dont know why)
//best range for mill, 24 bit pushout etc:
#define ARIT003_INTERNAL_MIN_RANGE 0x0000000001000000ull
#define ARIT003_INTERNAL_SHIFT_AMT 40ull
#define ARIT003_INTERNAL_SHIFT_MASK 0x000000FFFFFFFFFFull
#elif ARIT003_INTERNAL_PUSH_N_BITS == 32
#define ARIT003_INTERNAL_MIN_RANGE 0x0000000000010000ull
#define ARIT003_INTERNAL_SHIFT_AMT 32ull
#define ARIT003_INTERNAL_SHIFT_MASK 0x00000000FFFFFFFFull
#else
#error "Not supported"
#endif
inline int arit003_encode_bit(struct arit003_struct *enc_state, uint8_t bit_to_encode, uint16_t odds_of_zero_uint16){
    #if 0
    // ...
    #else
    uint64_t enc_range = (enc_state->x2 - enc_state->x1);
    
    if (enc_range < ARIT003_INTERNAL_MIN_RANGE) { // remaining range is small, push ~4 bytes out of either x1 or x2
        uint64_t enc_x1_n_msb = enc_state->x1 >> ARIT003_INTERNAL_SHIFT_AMT;
        uint64_t enc_x2_n_msb = enc_state->x2 >> ARIT003_INTERNAL_SHIFT_AMT;
        if(enc_x1_n_msb != enc_x2_n_msb) {
            //increase x1 or decrease x2
            //shifted out has to be equal, it isn't (corner case)
            //what is "shifted-out-value" ? there are two alternatives; top bits of x1, or top bits of x2
            //"what if we pushout x1" - then measure distance between x2 and the version of x2 that is --'ed until it has the same top bits as x2
            //"what if we pushout x2" - then measure distance between x1 and the version of x1 that is ++'ed until it has the same top bits as x1
            uint64_t largest_number_with_same_top_bits_as_x1 = (enc_x1_n_msb << ARIT003_INTERNAL_SHIFT_AMT) + ARIT003_INTERNAL_SHIFT_MASK;
            uint64_t dist_x2_to_largest_x1_top = enc_state->x2 - largest_number_with_same_top_bits_as_x1;
            uint64_t smallest_number_with_the_same_top_bits_as_x2 = enc_x2_n_msb << ARIT003_INTERNAL_SHIFT_AMT;
            uint64_t dist_x1_to_smallest_x2_top = smallest_number_with_the_same_top_bits_as_x2 - enc_state->x1;
            // which of those is smaller distance?
            if (dist_x1_to_smallest_x2_top < dist_x2_to_largest_x1_top) {
                enc_state->x1 = smallest_number_with_the_same_top_bits_as_x2;
            } else {
                enc_state->x2 = largest_number_with_same_top_bits_as_x1;
            }
#if ARIT003_USE_SLOW_ERROR_CHECKS >= 1
            uint64_t temp_new_range = enc_state->x2 - enc_state->x1;
            if (temp_new_range < 0x00010000ull) {
                printf2("ERR TOO SMALL temp_new_range=0x%16.16lX,\n", temp_new_range);
                return 4;
            }
#endif
        }
#if ARIT003_USE_SLOW_ERROR_CHECKS >= 1
        if(enc_state->x1 >> 48ull != enc_state->x2 >> 48ull) {
            printf2("ERROR, top 16 bits of x1 x2 different, pushout is invalid\n");
            return 6;
        }
#endif
#if ARIT003_INTERNAL_PUSH_N_BITS == 16
        enc_state->pushout_queue[enc_state->pushout_queue_count] = enc_state->x1 >> 56u;
        enc_state->pushout_queue[enc_state->pushout_queue_count+1] = (enc_state->x1 >> 48u);
        enc_state->x2 <<= 16ull;
        enc_state->pushout_queue_count += 2;
        enc_state->x1 <<= 16ull;
#elif ((ARIT003_INTERNAL_PUSH_N_BITS == 24) && (SUPPORT_RAW_BITS_V0003 == 1)) || 0
        enc_state->pushout_queue[enc_state->pushout_queue_count] = enc_state->x1 >> 56u;
        enc_state->pushout_queue[enc_state->pushout_queue_count+1] = (enc_state->x1 >> 48u);
        #if SUPPORT_RAW_BITS_V0003 == 1
		// When the first bit is pushed to the bitBuffer allocate a buffer 8 bytes ahead of current pushout_queue_count.
		//    If numFlushedBuffersOutstanding == 0 then set numFlushedBuffersOutstanding to 1.
		//    If numFlushedBuffersOutstanding > 0 then set numFlushedBuffersOutstanding+=1 and write to 8*numFlushedBuffersOutstanding.
		// When the 3'th arit0003 flush triggers write 2 bytes, skip 8*numFlushedBuffersOutstanding bytes, then write the last 1 byte.
		if(enc_state->countDownUntilStoreBuffer == 0) {
            enc_state->pushout_queue_count += enc_state->numFlushedBuffersOutstanding*8;
            enc_state->numFlushedBuffersOutstanding = 0;
            // Now check if there is a second lingering buffer after this flush.
            if(enc_state->buffersNextCountDown1 >= 0) {
            	enc_state->countDownUntilStoreBuffer = enc_state->buffersNextCountDown1;
            	enc_state->numFlushedBuffersOutstanding = enc_state->numFlushedBuffersOutstandingNext1;
            	// Now check if there is a third lingering buffer after this flush.
            	enc_state->buffersNextCountDown1 = enc_state->buffersNextCountDown2;
            	enc_state->numFlushedBuffersOutstandingNext1 = enc_state->numFlushedBuffersOutstandingNext2;
            	enc_state->numFlushedBuffersOutstandingNext2 = 0;
            	enc_state->buffersNextCountDown2 = -1;
            }
		}
		#endif
        {

        enc_state->pushout_queue[enc_state->pushout_queue_count+2] = (enc_state->x1 >> 40u);
        enc_state->x2 <<= 24ull;
        enc_state->pushout_queue_count += 3;
        enc_state->x1 <<= 24ull;
        }
        #if SUPPORT_RAW_BITS_V0003 == 1
        enc_state->countDownUntilStoreBuffer--;
        #endif
        
#elif ARIT003_INTERNAL_PUSH_N_BITS == 24
		((uint32_t *)&enc_state->pushout_queue[enc_state->pushout_queue_count])[0] = __builtin_bswap64(enc_state->x1);
		enc_state->x2 <<= 24ull;
		enc_state->pushout_queue_count += 3;
		enc_state->x1 <<= 24ull;
#else
		// 32 bit
		((uint32_t *)&enc_state->pushout_queue[enc_state->pushout_queue_count])[0] = __builtin_bswap64(enc_state->x1);
		enc_state->x2 <<= 32ull;
		enc_state->pushout_queue_count += 4;
		enc_state->x1 <<= 32ull;
#endif
        enc_range = (enc_state->x2 - enc_state->x1);
    }
#if ARIT003_USE_SLOW_ERROR_CHECKS >= 1
    if(enc_range < 1) {
        printf2("ERROR0; enc_range=0x%16.16lX, x1=0x%16.16lX, x2=0x%16.16lX\n", enc_range, enc_state->x1, enc_state->x2);
    } else if(enc_range < 0xFFFF) {
        printf2("ERROR1; enc_range=0x%16.16lX, x1=0x%16.16lX, x2=0x%16.16lX\n", enc_range, enc_state->x1, enc_state->x2);
    } else if(enc_range < 0xFFFFFFFF) {
        printf2("ERROR2; enc_range=0x%16.16lX, x1=0x%16.16lX, x2=0x%16.16lX\n", enc_range, enc_state->x1, enc_state->x2);
    }
#endif
#if 0
    uint64_t enc_odds_offset = (enc_range >> 16ull) * odds_of_zero_uint16; //float is ok for multiplication, division is never good
    uint64_t x1 = enc_state->x1;
    uint64_t x2 = enc_state->x2;
    uint64_t mid = x1 + enc_odds_offset;
    x1 = bit_to_encode ? mid : x1;
    x2 = bit_to_encode ? x2 : mid;
    enc_state->x1 = x1;
    enc_state->x2 = x2;
    enc_state->mid = mid;
    return 0;
#else
    #if ARIT003_INTERNAL_PUSH_N_BITS == 32
    uint64_t enc_odds_offset = (enc_range >> 8ull) * (odds_of_zero_uint16>>8); //float is ok for multiplication, division is never good
    #else
    uint64_t enc_odds_offset = (enc_range >> 16ull) * odds_of_zero_uint16; //float is ok for multiplication, division is never good
    #endif
    uint64_t mid = enc_state->x1 + enc_odds_offset;
    enc_state->x[(bit_to_encode)] = mid;
    #endif
    return 0;
#endif
}

int arit003_encode_finalize(struct arit003_struct *enc_state) {
    //finalize - meaning that bits not yet in pushout_queue need to go there now.
    //NOTE: VARIANT FOR FINALIZE! if decode is using dec_loaded_state >= dec_mid, which might be correct, then finalize needs to update midpoint
    //the last "mid_point" has not been updated, and actually is equal to one of the x'es
    uint64_t temp_range_x1_x2 = enc_state->x2 - enc_state->x1;
    uint64_t enc_odds_offset = temp_range_x1_x2 * 0.5;
    enc_state->mid = enc_state->x1 + enc_odds_offset;//TODO can we skip this update? and why implicit 0.5 odds?
    //NOTE: possibly finalize can choose if the last byte is saved or not, and possibly some LSB in the last byte are irrelevant
    #if SUPPORT_RAW_BITS_V0003 == 1
    //printf("finalize: countDownUntilStoreBuffer: %d\n", enc_state->countDownUntilStoreBuffer);
    int bytesUntilStoreBuffer = enc_state->countDownUntilStoreBuffer * 3 + 2;
    #endif

    for(int i = 7; i >= 0; i--) {
        uint64_t remainder = enc_state->mid >> (8ull * i);
        uint8_t pushout_val = remainder & 0x00FFull;
        //printf2("finalize: mid = %16.16lX, remainder=%16.16lX, i=%3d, pushout_val=%3i, 0x%2.2X\n", enc_state->mid, remainder, i, pushout_val, pushout_val);
        
        #if SUPPORT_RAW_BITS_V0003 == 1
        if(bytesUntilStoreBuffer == 0) {
            enc_state->pushout_queue_count += enc_state->numFlushedBuffersOutstanding*8;
            enc_state->numFlushedBuffersOutstanding = 0;
            
            if(enc_state->buffersNextCountDown1 >= 0) {
            	enc_state->countDownUntilStoreBuffer = enc_state->buffersNextCountDown1 - 1;
            	enc_state->numFlushedBuffersOutstanding = enc_state->numFlushedBuffersOutstandingNext1;
            	enc_state->buffersNextCountDown1 = enc_state->buffersNextCountDown2;
            	enc_state->numFlushedBuffersOutstandingNext1 = enc_state->numFlushedBuffersOutstandingNext2;
            	enc_state->numFlushedBuffersOutstandingNext2 = 0;
            	enc_state->buffersNextCountDown2 = -1;
            	
            	bytesUntilStoreBuffer = enc_state->countDownUntilStoreBuffer * 3 + 2;
            	//printf("finalize: bytesUntilStoreBuffer: %d\n", bytesUntilStoreBuffer);
            }
            
        }
        bytesUntilStoreBuffer--;
        #endif
        
        enc_state->pushout_queue[enc_state->pushout_queue_count] = pushout_val;
        enc_state->pushout_queue_count += 1;
    }
    
    #if SUPPORT_RAW_BITS_V0003 == 1
    if(bytesUntilStoreBuffer == 0) {
        enc_state->pushout_queue_count += enc_state->numFlushedBuffersOutstanding*8;
        enc_state->numFlushedBuffersOutstanding = 0;
        //printf("finalize: bytesUntilStoreBuffer: %d, enc_state->pushout_queue_count: %jd\n", bytesUntilStoreBuffer, enc_state->pushout_queue_count);
    }
    bytesUntilStoreBuffer--;
    #endif
    // TODO: possibly the lower bytes are not necessary, calculate that based on remaining enc_range (is it larger than FFFF etc).
    // TODO: Last decode step only needs to have range 2-ish, really, however it is best to avoid special cases (maximum saved is 2 bytes)
    return 0;
}

int arit003_decode_init(struct arit003_struct *dec_state) {
    for (uint32_t byte_i = 0; byte_i < 8 && byte_i < dec_state->pushout_queue_count; byte_i++) {
        dec_state->dec_loaded_state <<= 8ull; //room for one more
        dec_state->dec_loaded_state += dec_state->pushout_queue[dec_state->dec_pushout_queue_index];
        //printf2("initial pushout_queue pop: dec_pushout_queue_index=%4lu, val: 0x%2.2X, dec_loaded_state=0x%16.16lX\n", dec_state->dec_pushout_queue_index, dec_state->pushout_queue[dec_state->dec_pushout_queue_index], dec_state->dec_loaded_state);
        dec_state->dec_pushout_queue_index += 1;
    }
    while (dec_state->dec_pushout_queue_index < 8) { //if it didn't load full size, just bitshift so that MSBs are in the right location
        dec_state->dec_loaded_state <<= 8ull; //shifting in some zeroes at lsb
        dec_state->dec_pushout_queue_index += 1;
    }
    return 0;
}

// TODO consider that the decoded bit should be a &param, so that we can return error code int (like the other functions)

inline uint8_t arit003_decode_bit(struct arit003_struct *dec_state, uint16_t odds_of_zero_uint16) {
    uint64_t dec_range = dec_state->x2 - dec_state->x1;

    if(dec_range < ARIT003_INTERNAL_MIN_RANGE) { // running out of bits to read, take ~4 bytes from compressed buffer and put them in x1 or x2
        //printf2("pre  pushout, dec_range = 0x%16.16lX\n", dec_range);
        uint64_t dec_x1_n_msb = dec_state->x1 >> ARIT003_INTERNAL_SHIFT_AMT;
        uint64_t dec_x2_n_msb = dec_state->x2 >> ARIT003_INTERNAL_SHIFT_AMT;
        if(dec_x1_n_msb != dec_x2_n_msb) {
            //increase x1 or decrease x2
            //shifted out has to be equal, it isnt (corner case)
            //what is "shifted-out-value" ? there are two alternatives; top bits of x1, or top bits of x2
            //"what if we pushout x1" - then measure distance between x2 and the version of x2 that is --'ed until it has the same top bits as x2
            //"what if we pushout x2" - then measure distance between x1 and the version of x1 that is ++'ed until it has the same top bits as x1
            uint64_t largest_number_with_same_top_bits_as_x1 = (dec_x1_n_msb << ARIT003_INTERNAL_SHIFT_AMT) + ARIT003_INTERNAL_SHIFT_MASK;
            uint64_t dist_x2_to_largest_x1_top = dec_state->x2 - largest_number_with_same_top_bits_as_x1;
            uint64_t smallest_number_with_the_same_top_bits_as_x2 = dec_x2_n_msb << ARIT003_INTERNAL_SHIFT_AMT;
            uint64_t dist_x1_to_smallest_x2_top = smallest_number_with_the_same_top_bits_as_x2 - dec_state->x1;
            //printf2("largest_number_with_same_top_bits_as_x1=0x%16.16lX, smallest_number_with_the_same_top_bits_as_x2=0x%16.16lX, dist_x2_to_largest_x1_top=%lu, dist_x1_to_smallest_x2_top=%lu\n", largest_number_with_same_top_bits_as_x1, smallest_number_with_the_same_top_bits_as_x2, dist_x2_to_largest_x1_top, dist_x1_to_smallest_x2_top);
            // which of those is smaller distance?
            if(dist_x1_to_smallest_x2_top < dist_x2_to_largest_x1_top) {
                dec_state->x1 = smallest_number_with_the_same_top_bits_as_x2;
                //printf2("fixed dec_state->x1=0x%16.16lX, moved (lost) distance dist_x1_to_smallest_x2_top=0x%16.16lX, better than moving  dist_x2_to_largest_x1_top=0x%16.16lX\n", dec_state->x1, dist_x1_to_smallest_x2_top, dist_x2_to_largest_x1_top);
            } else {
                dec_state->x2 = largest_number_with_same_top_bits_as_x1;
                //printf2("fixed dec_state->x2=0x%16.16lX, moved (lost) distance  dist_x2_to_largest_x1_top=0x%16.16lX, better than moving dist_x1_to_smallest_x2_top=0x%16.16lX\n", dec_state->x2, dist_x2_to_largest_x1_top, dist_x1_to_smallest_x2_top);
            }
#if ARIT003_USE_SLOW_ERROR_CHECKS >= 1
            uint64_t temp_new_range = dec_state->x2 - dec_state->x1;
            //printf2("temp_new_range=0x%16.16lX,\n", temp_new_range);
            if(temp_new_range < 0x00010000ull) {
                printf2("ERR TOO SMALL temp_new_range=0x%16.16lX,\n", temp_new_range);
                return 4;
            }
#endif
            //before this, mid was used to set x1 or x2 so it is equal to one of them.
            //after this, mid will be calculated from x1 and x2.
            //typically we don't actually need mid to be part of the states
        } else {
            //printf2("no exception, pushout top dec_x1_n_32msb = 0x%16.16lX\n", dec_x1_n_32msb);
        }

#if ARIT003_INTERNAL_PUSH_N_BITS == 16
        dec_state->dec_loaded_state <<= 8ull;
        dec_state->dec_loaded_state += dec_state->pushout_queue[dec_state->dec_pushout_queue_index];
        dec_state->dec_pushout_queue_index += 1;
        dec_state->dec_loaded_state <<= 8ull;
        dec_state->dec_loaded_state += dec_state->pushout_queue[dec_state->dec_pushout_queue_index];
        dec_state->dec_pushout_queue_index += 1;
        dec_state->x1 <<= 16ull;
        dec_state->x2 <<= 16ull;
#elif ARIT003_INTERNAL_PUSH_N_BITS == 24
        dec_state->dec_loaded_state = (dec_state->dec_loaded_state << 24ull) | (dec_state->pushout_queue[dec_state->dec_pushout_queue_index] << 16ull) | (dec_state->pushout_queue[dec_state->dec_pushout_queue_index+1] << 8ull) | dec_state->pushout_queue[dec_state->dec_pushout_queue_index+2];
        dec_state->x1 <<= 24ull;
        dec_state->x2 <<= 24ull;
        dec_state->dec_pushout_queue_index += 3;

#elif ARIT003_INTERNAL_PUSH_N_BITS == 32
        dec_state->dec_loaded_state = __builtin_bswap64(((uint64_t *)&dec_state->pushout_queue[dec_state->dec_pushout_queue_index-4])[0]);
        dec_state->x1 <<= 32ull;
        dec_state->x2 <<= 32ull;
        dec_state->dec_pushout_queue_index += 4;
#else
#error "Error"
#endif
        dec_range = dec_state->x2 - dec_state->x1;
    }
#if ARIT003_USE_SLOW_ERROR_CHECKS >= 1
    if(dec_range < 65535) {
        printf2("ERROR; dec_range=%lu\n", dec_range);
    }
#endif

#if ARIT003_INTERNAL_PUSH_N_BITS == 32
    uint64_t dec_mid_point_in_range_int = (dec_range >> 8ull) * (odds_of_zero_uint16>>8);
#else
    uint64_t dec_mid_point_in_range_int = (dec_range >> 16ull) * odds_of_zero_uint16;
#endif
    uint64_t mid = dec_state->x1 + dec_mid_point_in_range_int;
    if(dec_state->dec_loaded_state >= mid) {
        dec_state->x1 = mid;
        return 1;
    } else {
        dec_state->x2 = mid;
        return 0;
    }
}
//TODO if i change order so that overflow happens at the end of the function, can then remove "mid" from the state struct.
