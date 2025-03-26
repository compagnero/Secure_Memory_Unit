#ifndef PARITY_CHECKER_HPP
#define PARITY_CHECKER_HPP
#include <map>
#include <systemc>
#include <systemc.h>
#include <unordered_map>

using namespace sc_core;

SC_MODULE(PARITY_CHECKER) {
    sc_in<sc_uint<8>> data_in;
    sc_in<sc_uint<32>> address_in;
    sc_out<bool> error;
    sc_in<bool> r, w,start_parity;
    sc_in<sc_uint<32>> fault;
    sc_in<bool> fault_Flag;

    //parity bits map: every byte has 8 parity bits, for 8 different addresses, made to save memory
    std::unordered_map<uint32_t, uint8_t> parity_map;

    void set_bit(std::unordered_map<uint32_t, uint8_t>& parity_map, uint32_t address) {
        uint32_t byte_index = address / 8;
        uint8_t bit_index = address % 8;

        // Set the bit in the corresponding byte
        parity_map[byte_index] |= (1 << bit_index);
         std::cout << "Set parity bit for address " << address << " | Byte Index: " 
              << byte_index << " | New Value: " << std::hex << (int)parity_map[byte_index] << std::endl;
    }

    void clear_bit(std::unordered_map<uint32_t, uint8_t>& parity_map, uint32_t address) {
        uint32_t byte_index = address / 8;
        uint8_t bit_index = address % 8;

        // Clear the bit in the corresponding byte
        parity_map[byte_index] &= ~(1 << bit_index);
         std::cout << "Cleared parity bit for address " << address << " | Byte Index: " 
              << byte_index << " | New Value: " << std::hex << (int)parity_map[byte_index] << std::endl;
    }

    bool check_bit(const std::unordered_map<uint32_t, uint8_t>& parity_map, uint32_t address) {
        uint32_t byte_index = address / 8;
        uint8_t bit_index = address % 8;

        // Check if the bit is set in the corresponding byte
        auto it = parity_map.find(byte_index);

        if (it != parity_map.end()) {
        bool bit = (it->second >> bit_index) & 1;
        std::cout << "Checking parity bit for address " << address 
                  << " | Value: " << bit << std::endl;
        return bit;
    }
    std::cout << "Address not found in parity map. Defaulting to 0." << std::endl;
    return 0; // Default to 0 if not found
    }

    SC_CTOR(PARITY_CHECKER) {
        SC_METHOD(behaviour);
        sensitive << start_parity.pos();
        dont_initialize();
    }

    bool calculate_parity(sc_uint<8> byte) {
        int count = 0;
        for (int i = 0; i < 8; ++i) {
            count += byte[i];
        }
        std::cout << "Byte: " << std::hex << byte << " | Parity Count: " << count 
              << " | Parity: " << (count % 2 != 0) << std::endl;
        return count % 2 != 0;
    }

    void behaviour() {
        uint32_t address = address_in.read();
        uint8_t data = data_in.read();
        bool calculated_parity = calculate_parity(data);

        if (r.read() || w.read()){
        if (r.read()) {
            bool stored_parity = check_bit(parity_map, address); //get the stored parity
            std::cout << "Reading from address " << address << " | Data: " << std::hex << (int)data << std::endl;
            if(calculated_parity != stored_parity){
                error.write(true);
                std::cout << "Error detected at address " << address << std::endl;
            } else {
                error.write(false);
            }
        } else {
            // No error when not checking, already done in run_simulation: we don't check when the byte is not written on before, but we calculate parity
            if (calculated_parity) {
                std::cout << "Writing to address " << address << " | Data: " << std::hex << (int)data << "parity bit 1" <<std::endl;
                set_bit(parity_map, address); // Set parity bit to 1
            } else {
                std::cout << "Writing to address " << address << " | Data: " << std::hex << (int)data << "parity bit 0" << std::endl;
                clear_bit(parity_map, address); // Set parity bit to 0
            }
        }
        }

        
         if (fault_Flag.read()){ // fault injection
            std::cout<<"!!I'm in the case where faultBit is 8 and I'm inverting the bit in Parity Checker!!"<<std::endl;
            if (check_bit(parity_map, fault.read())) {
                std::cout<<"---------- The bit was : "<< check_bit(parity_map, fault.read()) << std::endl;
                clear_bit(parity_map, fault.read());  // Clear parity bit
                std::cout<<"---------- And the bit now is : "<< check_bit(parity_map, fault.read()) << std::endl;
            } else {
                std::cout<<"---------- The bit was : "<< check_bit(parity_map, fault.read()) << std::endl;
                set_bit(parity_map, fault.read()); // Set parity bit
                std::cout<<"---------- And the bit now is : "<< check_bit(parity_map, fault.read()) << std::endl;
            }
        }
    }

    bool getParityBit(uint32_t address){
        return check_bit(parity_map, address);
    }


};
#endif




