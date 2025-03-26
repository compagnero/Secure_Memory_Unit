#ifndef ENCRYPT_DECRYPT_HPP
#define ENCRYPT_DECRYPT_HPP
#include <map>
#include <array>
#include <systemc>
#include <systemc.h>
#include "prng.hpp"
using namespace sc_core;
SC_MODULE(ENCRYPT_DECRYPT) {
  
      sc_in<bool> w, r, start_encrypt_decrypt;
      sc_in<sc_uint<32>> wdata;
      sc_out<sc_uint<8>> data_verschluesselt[4];
      sc_out<sc_uint<8>> data_entschluesselt[4];
      sc_uint<8> encryption_key;
      sc_in<sc_uint<8>> data_to_be_decrypted[4];// Coming from memory
      sc_in<sc_uint<32>> addr_in;
      
      // Saving encryption Key for decryption 
      std::map<std::array<sc_uint<32>, 4>,sc_uint<8> > key_map;
      PRNG prng;

    SC_CTOR(ENCRYPT_DECRYPT) : encryption_key(0){
        SC_METHOD(encrypt_or_decrpyt);
         sensitive << start_encrypt_decrypt.pos();
    }
     
    void encrypt_or_decrpyt() {
            if(start_encrypt_decrypt.read()) {
            //Schreibzugriff
            if (w.read()==true){
              
          sc_uint<8> wdata_divided[4];
              //Seed setzen und Zufallszahl generieren, wenn kein Encryption Key gesetzt ist
                setEncryptionKey (prng.createNumber().range(7,0));
                 
                key_map[{addr_in.read(),addr_in.read()+1,addr_in.read()+2,addr_in.read()+3}]= encryption_key;
                std::cout << "Encryption key is :"<<encryption_key<<std::endl;
            
              // Dividieren der Data  und Verschlüsselung 
            for (size_t i = 0; i < 4; i++)
            {  // Verschlüsselung 
               wdata_divided[i]=  wdata.read().range((i + 1) * 8 - 1, i * 8)^encryption_key;
            //Verschlüsselt_Data senden
             data_verschluesselt[i].write(wdata_divided[i]);
            
            }
            
            }
            //Lesezugriff
            else if (r.read()==true)

            { if(key_map.count({addr_in.read(),addr_in.read()+1,addr_in.read()+2,addr_in.read()+3})){ // if data  already exisits 
              setEncryptionKey(key_map[{addr_in.read(),addr_in.read()+1,addr_in.read()+2,addr_in.read()+3}]);
                  //Entschlüsselung und Senden
                  for (size_t i = 0; i < 4; i++)
               { data_entschluesselt[i].write( data_to_be_decrypted[i].read() ^ encryption_key);}
                
             
              }
              else { // if data doesnt exist in that address 
                for (size_t i = 0; i < 4; i++)
                   {data_entschluesselt[i].write(0);}

              }
          
            }
            }
        }

           
            
     
    
    void setEncryptionKey(sc_uint<32> key) {
        encryption_key = key;
    }
      sc_uint<32> getEncryptionKey(){
         
        return encryption_key ;
      }
    };

#endif