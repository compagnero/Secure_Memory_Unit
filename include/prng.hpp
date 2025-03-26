#ifndef PRNG_HPP
#define PRNG_HPP
#include <systemc>
#include <systemc.h>
//this is a helper class, not a SystemC module
//used to generate random numbers for the Address Scrambler and Encrypt Decrypt module
//a Linear Congruential Generator (LCG) pseudo-random number generator, generates a random number based on a given seed

class PRNG {
  public:  
    PRNG() : currentNumber(1234) {}
   explicit PRNG(sc_uint<32> seed) 
       : currentNumber(seed) {}

    //generates the next random number
    sc_uint<32> createNumber() {
        //linear congruential generator, formula source: https://en.wikipedia.org/wiki/Linear_congruential_generator
        currentNumber = (currentNumber * 1664525 + 1013904223) % 4294967296;
        return currentNumber;
    }
    
    //sets the seed for the random number generator
    void set_seed(sc_uint<32> seed) {
        currentNumber = seed;
    }
 private:
    sc_uint<32> currentNumber;
};
#endif