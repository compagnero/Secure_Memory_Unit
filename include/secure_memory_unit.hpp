#ifndef SECURE_MEMORY_UNIT_HPP
#define SECURE_MEMORY_UNIT_HPP
#include <systemc>
#include "address_scrambler.hpp"
#include "encrypt_decrypt.hpp"
#include "parity_checker.hpp"
#include "relevant_structs.h"
using namespace sc_core;

SC_MODULE(SECURE_MEMORY_UNIT)
{
  // Ports and signals
  sc_in<sc_uint<32>> addr, wdata, fault;
  sc_in<bool> clk, r, w;
  sc_in<sc_bv<4>> faultBit;
  sc_out<sc_uint<32>> rdata;
  sc_out<bool> ready, error;

  // Internal variables
  std::map<sc_uint<32>, sc_uint<8>> memory; // memory idea gotten from the homework T8-4 RISC-V Main Memory, main_memory.hpp, Solution, Line 21
  uint32_t seed;// seed for the prng
  uint8_t endianness;
  uint32_t latencyScrambling;
  uint32_t latencyEncrypt;
  uint32_t latencyMemoryAccess;
  uint32_t cycleCounter;
  uint32_t primitiveGateCounter;

  // Signals for the modules
  sc_signal<sc_uint<32>> scrambled_addresses_signals[4];
  sc_signal<sc_uint<8>> data_to_be_decrypted_fromSMU[4];
  sc_signal<sc_uint<8>> data_entschluesselt_signals[4];
  sc_signal<sc_uint<8>> data_verschluesselt_signals[4];
  sc_signal<sc_uint<8>> data_in_signal_paritychecker;
  sc_signal<sc_uint<32>> address_in_signal_paritychecker;

  sc_signal<bool> start_encrypt_decrypt, start_scramble, start_parity, fault_Flag; // signals to trigger the starting of modules

  // Submodules
  ADDRESS_SCRAMBLER address_scrambler;
  ENCRYPT_DECRYPT encrypt_decrypt;
  PARITY_CHECKER parity_checker;
  const uint32_t MAX_MEMORY_ENTRIES = UINT32_MAX;
  bool ready_value; 

  SC_CTOR(SECURE_MEMORY_UNIT) : address_scrambler("address_scrambler"), encrypt_decrypt("encrypt_decrypt"),
                                parity_checker("parity_checker")
  {

    // initialize submodule signals to false
    start_encrypt_decrypt.write(0);
    start_scramble.write(0);
    start_parity.write(0);

    // bind signals to the submodules
    address_scrambler.start_scramble(start_scramble);
    encrypt_decrypt.start_encrypt_decrypt(start_encrypt_decrypt);
    parity_checker.start_parity(start_parity);

    address_scrambler.prng.set_seed(seed);
    encrypt_decrypt.prng.set_seed(seed);
    // binding  scrambler input
    address_scrambler.w(w);
    address_scrambler.r(r);
    address_scrambler.address_in(addr);

    // binding scrambler output, (Comparator)+(Increment)+(Branch)≈11+40+5=56 gates. 56x4=224 primitive gates
    for (sc_uint<8> i = 0; i < 4; i++)
    {
      address_scrambler.scrambled_subaddresses[i](scrambled_addresses_signals[i]);
    }

    // binding  parity checker input
    parity_checker.data_in(data_in_signal_paritychecker);
    parity_checker.address_in(address_in_signal_paritychecker);
    parity_checker.fault(fault);
    parity_checker.fault_Flag(fault_Flag);
    parity_checker.r(r);
    parity_checker.w(w);
    // binding partiy checker output
    parity_checker.error(error);

    // binding encrypt_decrypt input
    encrypt_decrypt.wdata(wdata);
    encrypt_decrypt.r(r);
    encrypt_decrypt.w(w);
    encrypt_decrypt.addr_in(addr);

    // binding encrypt_decrypt output, 224 primitive gates
    for (sc_uint<8> i = 0; i < 4; i++) //224 primitive gates
    {
      encrypt_decrypt.data_to_be_decrypted[i](data_to_be_decrypted_fromSMU[i]);
    }
    for (sc_uint<8> i = 0; i < 4; i++)
    {
      encrypt_decrypt.data_entschluesselt[i](data_entschluesselt_signals[i]);
      encrypt_decrypt.data_verschluesselt[i](data_verschluesselt_signals[i]);
    }


    SC_CTHREAD(operations, clk.pos());
  }

  void operations()
  {
    std::cout << "I'm in operations!!" << std::endl;
    std::cout << "ready ? : " << ready.read() << std::endl;

    while (true)
    {
      wait();
      if (ready_value != true) // 4+1=5 primitive gates
      {
        //25 primitive gates
        if (w.read() == true && r.read() == false && fault.read() != UINT32_MAX) //2+1+63=66 primitive gates
        {
          std::cout << "I'm in operation: write & fault!!" << std::endl;
          write_op(); 
          inject_fault();
        }
        else if (w.read() == true && r.read() == false) //2 primitive gates
        {
          std::cout << "I'm in operation: write!!" << std::endl;
          write_op();
        }
        else if (r.read() == true && w.read() == false && fault.read() != UINT32_MAX) //2+1+63=66 primitive gates
        {
          std::cout << "I'm in operation: read & fault!!" << std::endl;
          read_op();
          inject_fault();
        }
        else if (r.read() == true && w.read() == false) // 2 primitive gates
        {
          std::cout << "I'm in operation: read!!" << std::endl;
          read_op();
        }

        else if (fault.read() != UINT32_MAX) //63 primitive gates
        {
          std::cout << "I'm in fault!!" << std::endl;
          inject_fault();
        } else {
          ready.write(true);
          ready_value = true;
          std::cout << "Fault Request but fault address is UINT32_MAX" << std::endl;
        }
      }
    }
  } 

  void write_op()
  {
    uint32_t latency = std::max(latencyEncrypt, latencyScrambling); // encryption & scrmabling happen at the same time

    start_scramble.write(true);        // start scrambling
    start_encrypt_decrypt.write(true); // start encryption

    for (size_t i = 0; i < latency; i++) //160 for addition + 68 for comparison = 228 for each iteration -> total = 228×latency primitive gates
    {
      wait(); 
    }
    start_scramble.write(false);        // end scrambling
    start_encrypt_decrypt.write(false); // end encryption

    handle_memory_overflow();

    //4 primitive gates for if else + 1 gate for comparison = 5 primitive gates
    if (endianness == 1) {
      for (sc_uint<8> i = 0; i < 4; i++) // 224 primitive gates
      {
        memory[scrambled_addresses_signals[i].read()] = data_verschluesselt_signals[3 - i].read();
        for (size_t i = 0; i < latencyMemoryAccess; i++) //4x228×latency primitive gates
        {
          wait(); // latency for storing in the memory, latency idea gotten from the homework T8-4 RISC-V Main Memory, main_memory.hpp, Solution, Line 45-47
        }
         std::cout << "(W) memory encrypted at byte " << i << "in the address: "<<scrambled_addresses_signals[i].read() << " is : " << memory[scrambled_addresses_signals[i].read()] << std::endl;
      }
    }

    else {
      for (size_t i = 0; i < 4; i++)
      {
        memory[scrambled_addresses_signals[i].read()] = data_verschluesselt_signals[i].read();
        for (size_t i = 0; i < latencyMemoryAccess; i++)
        {
          wait(); // latency for storing in the memory
        }
        std::cout << "(W) memory encrypted at byte " << i << "in the address: "<<scrambled_addresses_signals[i].read() << " is : " << memory[scrambled_addresses_signals[i].read()] << std::endl;
      }
    }

    for (sc_uint<8> i = 0; i < 4; i++) // 224 primitive gates
    {
      fault_Flag.write(false);
      data_in_signal_paritychecker.write(data_verschluesselt_signals[i].read());
      address_in_signal_paritychecker.write(scrambled_addresses_signals[i].read());
      wait();                  
      start_parity.write(true); // parity checker module called -- trigger it again for every byte
      wait();
      start_parity.write(false);
    }

    if (fault.read() == UINT32_MAX && ready_value == false) // 4 + 63 + 1+ 1 = 69 primitive gates
    {
      ready.write(true);
      ready_value = true;
      std::cout << "ready is set after write!!" << std::endl;
    }
  }

  void read_op()
  { 
    start_scramble.write(true); // start scrambling to find scrambled addresses
    for (size_t i = 0; i < latencyScrambling; i++) //228×latency primitive gates
    {
      wait(); // wait for scrambling
    }
    start_scramble.write(false);

    for (sc_uint<8> i = 0; i < 4; i++) // 224 primitive gates
    {
      if (endianness == 1) // 4 primitive gates
      {

        uint32_t memory_encrypted = memory[scrambled_addresses_signals[3 - i].read()];
        for (size_t i = 0; i < latencyMemoryAccess; i++) //4x228×latency primitive gates
        {
          wait(); // latency for accessing the memory
        }

        std::cout << "(R) memory encrypted at byte " << i << " is : " << memory_encrypted << std::endl;

        data_to_be_decrypted_fromSMU[i].write(memory_encrypted); // retrieving data from the memory

        std::cout << "(R) memory to be decrypted at byte " << i << " is : " << data_to_be_decrypted_fromSMU[i] << std::endl;
      }
      else 
      {
        uint32_t memory_encrypted = memory[scrambled_addresses_signals[i].read()];
        for (size_t i = 0; i < latencyMemoryAccess; i++)
        {
          wait(); // latency for accessing the memory
        }
        std::cout << "(R) memory encrypted at byte " << i << " is : " << memory_encrypted << std::endl;
        data_to_be_decrypted_fromSMU[i].write(memory_encrypted);
      }
    }

    start_encrypt_decrypt.write(true); // start decryption
    for (size_t i = 0; i < latencyEncrypt; i++) //228×latency primitive gates
    {
      wait(); // wait for decryption
    }
    for (size_t i = 0; i < 4; i++) 
    {
      std::cout << "(R) memory decrypted at byte " << i << " is : " << data_entschluesselt_signals[i].read() << std::endl;
    }

    start_encrypt_decrypt.write(false); // end decryption

    sc_uint<32> readData = 0;

    for (sc_uint<8> i = 4; i > 0; i--) //224 primitive gates
    {
      readData += data_entschluesselt_signals[i - 1].read() << ((i - 1) * 8); //32-bit addition = 160 gates => 4 times => 640 gates
    }

    bool error_value = 0;
    sc_uint<8> i = 0;
    while (!error_value && i < 4) //1 gate not + 15 gates for comparison + 1 and = 17 primitive gates * 4 = 68 primitive gates
    { // if error detected once in the 4 bytes, error is set
      fault_Flag.write(false);
      data_in_signal_paritychecker.write(data_to_be_decrypted_fromSMU[i].read());
      address_in_signal_paritychecker.write(scrambled_addresses_signals[i].read());
      wait();
      start_parity.write(true); // parity checker module triggered for byte i
      wait();
      error_value = parity_checker.error.read();
      i++; //8 bit adder 40 gates * 4 = 160 gates
      start_parity.write(false); // parity checker module ended
    } 
    start_parity.write(false); // make sure parity checker module ends in the event of breaking in the middle of the while without ending the module

    if (error_value) // 4 primitive gates
    {
      rdata.write(0);
      std::cout << "error detected in read operation and rdata is 0" << std::endl;
    }
    else
    {
      rdata.write(readData);
      std::cout << "final readData inside read operation : " << readData << std::endl;
    }
    if (fault.read() == UINT32_MAX && ready_value == false) // 4 + 63 + 1+ 1 = 69 primitive gates
    {
      ready.write(true);
      ready_value = true;
      std::cout << "ready is set after read!!" << std::endl;
    }
  }

  void inject_fault()
  {
    if (faultBit.read().to_uint() <= 7) // 4 gates for if + 27 gates for comparison = 31 primitive gates 
    {
      std::cout << "++++++++fault address : " << std::hex << fault.read() << std::endl;
      sc_uint<8> byte_to_be_changed = memory[fault.read()];
      std::cout << "++++++++byte_to be changed : " << std::hex << memory[fault.read()] << std::endl;

      for (size_t i = 0; i < latencyMemoryAccess; i++) //228×latency primitive gates
      {
        wait(); // latency for accessing the memory
      }

      sc_uint<8> mask = 1 << faultBit.read().to_uint(); // 12 primitive gates
      sc_uint<8> new_byte = byte_to_be_changed ^ mask; // 8 gates for xor = 8 primitive gates
      memory[fault.read()] = new_byte;
      for (size_t i = 0; i < latencyMemoryAccess; i++) //228×latency primitive gates
      {
        wait(); // latency for accessing the memory
      }
      std::cout << "++++++++new_byte after fault injection : " << new_byte << " | fault_bit : " << faultBit.read() << std::endl;

    }
    else if (faultBit.read().to_uint() == 8) //7 gates for comparison
    {
      fault_Flag.write(true);
      wait();
      start_parity.write(true);
      wait();
      start_parity.write(false);
    }
    ready.write(true);
    ready_value= true;
  
      std::cout << "ready is set after fault!!" << std::endl;
  }

  uint8_t getByteAt(uint32_t physicalAddress)
  {
    if (memory.find(physicalAddress) != memory.end())
    {
      return memory[physicalAddress];
    }
    else
    {
      //if byte not found, return 0 
      return 0;
    }
  }

  void handle_memory_overflow()
  {
    if (memory.size() >= MAX_MEMORY_ENTRIES)
    { 
      throw std::overflow_error("Buffer overflow: Memory capacity exceeded (2^32 entries).");
    }
  }

  // setter for setting the seed from the outside (run_simu .cpp)
  void setSeed(uint32_t seed_tobe_set)
  {
    seed = seed_tobe_set;
    address_scrambler.prng.set_seed(seed);
    encrypt_decrypt.prng.set_seed(seed);
  }

  void setScramblingKey(sc_uint<32> key)      
  {
    address_scrambler.setScramblingKey(key);
  }

  void setEncryptionKey(sc_uint<32> key)
  {
    encrypt_decrypt.setEncryptionKey(key);
  }
};
#endif