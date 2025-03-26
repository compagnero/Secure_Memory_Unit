#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <systemc.h>
#include <systemc>
#include "../include/secure_memory_unit.hpp"
#include "../include/relevant_structs.h"
#include "../include/run_simulation.hpp"


struct Result run_simulation(
    uint32_t cycles,
    const char* tracefile,
    uint8_t endianness,
    uint32_t latencyScrambling,
    uint32_t latencyEncrypt,
    uint32_t latencyMemoryAccess,
    uint32_t seed,
    uint32_t numRequests,
    struct Request* requests
) {

    struct Result result = {0, 0, 0}; // Initialize result counters

    // Declare signals for the Secure Memory Unit
    sc_signal<sc_uint<32>> addr_signal, wdata_signal, fault_signal;
    sc_signal<bool> r_signal, w_signal;
    sc_signal<sc_bv<4>> faultBit_signal;
    sc_signal<sc_uint<32>> rdata_signal;
    sc_signal<bool> ready_signal, error_signal;

    // Instantiate Secure Memory Unit
    SECURE_MEMORY_UNIT smu("SMU");

    sc_clock clk_signal("clk_signal", 1, SC_NS);
    smu.clk(clk_signal);  // Connect clock to SMU
    
    smu.addr(addr_signal);
    smu.wdata(wdata_signal);
    smu.fault(fault_signal);
    smu.r(r_signal);
    smu.w(w_signal);
    smu.faultBit(faultBit_signal);
    smu.rdata(rdata_signal);
    smu.ready(ready_signal);
    smu.error(error_signal);


    //initialise signal
    addr_signal.write(0);
    wdata_signal.write(0);
    fault_signal.write(UINT32_MAX); 
    r_signal.write(false);
    w_signal.write(false);
    faultBit_signal.write(0);

    sc_trace_file * tracefile1 = nullptr;
    if(tracefile && strlen(tracefile) > 0){
        tracefile1 = sc_create_vcd_trace_file(tracefile);
        if(!tracefile1){
            std::cerr << "Error: Unable to create trace file" << std::endl;
        }
        else {
            std::cout << "Trace file created successfully" << std::endl;
     sc_trace (tracefile1, addr_signal, "addr_signal");
     sc_trace (tracefile1, wdata_signal, "wdata_signal");
     sc_trace (tracefile1, fault_signal, "fault_signal");
     sc_trace (tracefile1, r_signal, "read_signal");
     sc_trace (tracefile1, w_signal, "write_signal");
     sc_trace (tracefile1,faultBit_signal, "fault_Bit");
     sc_trace (tracefile1, rdata_signal, "read_data");
     sc_trace (tracefile1, error_signal, "error_signal");
     sc_trace (tracefile1, ready_signal, "ready_signal");
        }
    }
    // Initialize parameters
    smu.endianness = endianness;
    smu.latencyScrambling = latencyScrambling;
    smu.latencyEncrypt = latencyEncrypt;
    smu.latencyMemoryAccess = latencyMemoryAccess;

    // Seed for PRNG
    smu.setSeed(seed); 
    ready_signal.write(false);//ready starts with 0
    smu.ready_value = false;

    sc_uint<32> writeOpCounter = 0;
    sc_uint<32> readOpCounter = 0;
    sc_uint<32> faultDataOpCounter = 0;
    sc_uint<32> faultParityOpCounter = 0;


     // Iterate over requests
    for (uint32_t i = 0; i < numRequests && result.cycles < cycles; i++) {
        struct Request *req = &requests[i];
        
        std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
        std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
        std::cout<<"I'm in the "<<std::dec<<i+1<<"th request now!"<<std::endl; 


        // Update signals based on the request
        smu.error.write(false);
        addr_signal.write(req->addr);
        wdata_signal.write(req->data);
        fault_signal.write(req->fault);
        faultBit_signal.write(req->faultBit);
        ready_signal.write(false);
        smu.ready_value = false;
        r_signal.write(req->r);
        w_signal.write(req->w);
        sc_start(1, SC_NS); // Simulate one clock cycle
        result.cycles++;
        std::cout<<"ready_value is : "<<ready_signal.read()<<" | r is : "<< r_signal.read()<<" | w is "<< w_signal.read() <<std::endl;





        // Wait for the SMU to finish the operation
        while (!smu.ready_value && result.cycles < cycles) {
            
            sc_start(1, SC_NS); // Simulate one clock cycle
            result.cycles++;
        }
        
       
        std::cout<< "error ? " << smu.error.read()<<std::endl;

        if(smu.error.read()){
            result.errors++;
        }
        
        /*

        *** Primitive gate count - Key approximations ***

        For every memory access to map : cost_of_gates is the number of gates used for one memory access:  67*(nbr of entries in the map) in the worst case
        - cost_of_gates_encrypt_map: the number of entries in the map is the number write requests only
        - cost_of_gates_parity_map: the number of entries is the number of fault(in parity; when parity bit is 8) and write
        - cost_of_gates_scrambling_map: the number of entries is the number of write and read requests
        - cost_of_gates_memory_map: the number of entries is the number of write and fault in data requests 

        SMU: without taking submodules into consideration
        672 (for the first 3 loops before operations method) + 229 (for one while iteration) + 
        FOR WRITE: (228×latency)+(latency memory access×912)+ 4*cost_of_gates_memory_map + 224 forloop (for memory access)+192 subtraction (3-i) extra gates for else part+ 69 if condition +224 loop of parity 
        FOR READ:  (228×latencyScrambling) +  (224 for loop+ 4 if) +192 [subtraction (3-i)] extra gates for else part + (912×latencyMemoryAccess)+4*cost_of_gates_memory_map+ (228×latencyEncrypt)  + 224 for loop + 640 for addition in read data + 228 gates in while loop(parity loop) + 69 gates for ifs(setting read part)+4(if error is 1)
        FOR FAULT in data: 31 for if + (2 x 228 x latency memory access) + 12 mask + 8 xor + 67*( writeOpCounter+faultParityOpCounter)
        FOR FAULT in parity: 31 for if + 7 for comparison 
       
        Address Scrambler: Total Gates : 6998+67*map entry count -> explanation in the slides, primitve gate count  part, simulation
        Encrypt Decrypt: Total: 6326 + (67* (writeOpCounter)) -> explanation in the slides, primitve gate count part , simulation
        Parity Checker: Total Gates : 44+67*map entry count -> explanation in the slides, primitve gate count part, simulation

      

        */

            int write_gates = 709 + 228 * std::max(latencyEncrypt, latencyScrambling) + 4*228*latencyMemoryAccess + 4*67*(writeOpCounter+faultDataOpCounter);
            int read_gates = 1585 + 228*latencyScrambling + 4*228*latencyMemoryAccess + 228*latencyEncrypt + 4*67*(writeOpCounter+faultDataOpCounter);
            int fault_data_gates = 51 +2*228*latencyMemoryAccess+ 67*(writeOpCounter+faultDataOpCounter);
            int fault_parity_gates = 38;
            int parity_checker_module_gates = 44 + (67* (writeOpCounter+faultParityOpCounter)); 
            int encrypt_module_gates = 6326 + (67* (writeOpCounter));
            int address_scrambler_module_gates = 6998 + (67* (writeOpCounter+readOpCounter)) + 192;

            int total_write_gates = write_gates + address_scrambler_module_gates + encrypt_module_gates + 4*(parity_checker_module_gates);
            int total_read_gates = read_gates + address_scrambler_module_gates + encrypt_module_gates + 4*(parity_checker_module_gates);
            int total_fault_gates = fault_data_gates + fault_parity_gates + parity_checker_module_gates;

            result.primitiveGateCount += total_fault_gates+total_read_gates+total_write_gates + 911;


        if (req->r) {//Read with no Fault
            readOpCounter++;
        } 
        else if (req->w ){ //Write with no Fault
            writeOpCounter++;
        } 
        
        if (req->fault != __UINT32_MAX__ ){//Only Fault
            if ( req->faultBit != 8) {
                faultDataOpCounter++;
            } else {
                faultParityOpCounter++;
            }
        }  
    }

    

    if(tracefile1) {
    sc_close_vcd_trace_file (tracefile1) ; // closing the tracefile
}
    return result;
}

int sc_main(int argc, char* argv[])
{
   
    return 1;
}