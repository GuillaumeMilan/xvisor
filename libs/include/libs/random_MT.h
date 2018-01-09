#ifndef _RANDOM_MT_H__
#define _RANDOM_MT_H__

#define NN 312
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */

/* initializes mt[NN] with a seed */
void init_genrand64(unsigned long long seed);

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* recommended init is:
 *
 * unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
 * init_by_array64(init, length);
 */
void init_by_array64(unsigned long long init_key[],
                     unsigned long long key_length);

/* generates a random number on [0, 2^64-1]-interval */
unsigned long long genrand64_int64(void);

/* generates a random number on [0, 2^63-1]-interval */
long long genrand64_int63(void);

#endif
