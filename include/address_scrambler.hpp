#ifndef ADDRESS_SCRAMBLER_HPP
#define ADDRESS_SCRAMBLER_HPP
#include <map>
#include <systemc>
#include <systemc.h>
#include "prng.hpp"

using namespace sc_core;

SC_MODULE(ADDRESS_SCRAMBLER) {
    sc_in<sc_uint<32>> address_in;
    sc_out<sc_uint<32>> scrambled_subaddresses[4];
    sc_in<bool> w ,r, start_scramble;
    sc_uint<32> scrambling_key;
    std::map<sc_uint<32>,sc_uint<32> > key_map;
    PRNG prng;

    SC_CTOR(ADDRESS_SCRAMBLER) : scrambling_key(0) {
        SC_METHOD(scramble_address);
        sensitive << start_scramble.pos();
    }

    void scramble_address() {
    
           if (start_scramble.read() && (w.read()==true xor r.read()==true)) {
            //read address
            sc_uint<32> address = address_in.read();
                //retrieve the Scrambling Key from the map
                 if (key_map.count(address)) {
                  scrambling_key = key_map[address];
                }
                //if the key is not in the map, generate a new one using PRNG
                else {
                     scrambling_key = prng.createNumber();
                     key_map[address] = scrambling_key;
                }
           
               //generate 4 scrambled subaddresses
            for(int i = 0; i < 4; i++) {
                //generate subaddresses
                sc_uint<32> subaddress = address + i;
                //XOR each subaddress with the scrambling key
                sc_uint<32> scrambled = subaddress ^ scrambling_key;
                //each scrambled subaddress is written to the corresponding output
                scrambled_subaddresses[i].write(scrambled);
                std::cout << "Subaddress: " << subaddress << " Scrambling key: " << scrambling_key << " Scrambled subaddress: " << scrambled<< std::endl;
            }
        }
       
           
    }
    void setScramblingKey(sc_uint<32> key) {
        scrambling_key = key;
    }
    
    uint32_t getScramblingKey(){
        return scrambling_key;
        }

};
#endif