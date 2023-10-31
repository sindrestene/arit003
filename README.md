This is the implementation of a fast binary arithmetic encoder and decoder. 

The original use-case was predictive image compression on bucketed pixels, but also works with ML based predictors, as well as any other program that calculates odds for the next bit based on previous bits.

The dependencies are minimal, compatibility is high.

The typical use-case is if you have a compression algorithm that wants to store one bit at a time, and has an idea about what the odds are of the bit being 0 or 1.

If the prediction of the odds is good, the cost of storing the bit goes down. In this way, storing one bit can cost more or less than 1 bit of space, depending on the accuracy of the provided odds.

During the implementation, first I decided on a minimum acceptable storage overhead, then made no compromises regarding the encoding/decoding speed.
Choices that have been tuned:
* Number of remaining bits in x1/x2 before 

For the reader, if you look at arit003_encode_bit() and arit003_decode_bit(), you may see how they match step-by-step.

## build
```
gcc -g -O3 -march=native arit003*.c -lm -o arit003_tests
```
or use CMake.
