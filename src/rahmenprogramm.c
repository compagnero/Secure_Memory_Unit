#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "../include/relevant_structs.h"
#include "../include/run_simulation.hpp"

//length of a Request line logically would not exceed 256
#define MAX_LINE_LENGTH 256
#define MAX_REQUESTS __UINT32_MAX__

extern struct Result run_simulation(
    uint32_t cycles,
    const char* tracefile,
    uint8_t endianness,
    uint32_t latencyScrambling,
    uint32_t latencyEncrypt,
    uint32_t latencyMemoryAccess,
    uint32_t seed,
    uint32_t numRequests,
    struct Request* requests);

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] <input_file>\n", prog_name);
    printf("Options:\n");
    printf("  --cycles, -c <number>             Number of simulation cycles (default: 1000)\n");
    printf("  --tf, -t <tracefile>              Path to the trace file (optional)\n");
    printf("  --endianness, -e <number>         Endianness (default: 0)\n");
    printf("  --latency-scrambling, -s <number> Latency for address scrambling (default: 1)\n");
    printf("  --latency-encrypt, -l <number>    Latency for encryption/decryption (default: 3)\n");
    printf("  --latency-memory-access, -m <number> Latency for memory access (default: 4)\n");
    printf("  --seed, -r <number>               Seed for the pRNG (default: 1234)\n");
    printf("  --help, -h                       Display this help message\n");
    printf("Arguments:\n");
    printf("  <input_file>                     Path to the input CSV file with requests\n");
}

uint32_t count_requests(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    uint32_t count = 0;

    while (fgets(line, sizeof(line), file)) {

        //maximum line length shouldn't be exceeded, output error if so.. why ? to not accept requests that have too many unnecessary zeros in hex numbers for example
        if (line[strlen(line) - 1] != '\n' && !feof(file)) {
            fprintf(stderr, "Error: Line exceeds maximum allowed length of %d characters.\n", MAX_LINE_LENGTH - 1);
            fclose(file);
            return EXIT_FAILURE;
        }

        if (strspn(line, " \t\r\n") == strlen(line) || line[0] == '#') {
            continue; // Skip empty lines or comment lines
        }
        count++;
    }

    // error if number of requests is too much(more than 2^32 requests)
    if (count >= MAX_REQUESTS) {
        fprintf(stderr, "Exceeded maximum number of requests (%d)\n", MAX_REQUESTS);
        fclose(file);
        return -1;
        }

    fclose(file);
    return count;
}

     
    int parse_requests(const char *filename, struct Request *requests) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    uint32_t countReq = 0; 
   
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        // Skip empty lines or comments
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        struct Request req = {0, 0, 0, 0, __UINT32_MAX__, 0};
        char type;
        int parsed;

       
        // Tokenize the line
        char *tokens[5] = {NULL};
        int token_count = 0;
        int i = 0;

        // Tokenizing manually while preserving empty fields
        char *start = line;
        while (start != NULL && token_count < 5) {
            char *end = strchr(start, ',');  // Find the next comma

            if (end != NULL) {
                *end = '\0';  // Temporarily terminate the string at the comma
            }

            // If start points to a valid string, assign it to the token
            if (start != NULL && *start != '\0') {
                tokens[token_count++] = start;
            } else {
                tokens[token_count++] = "empty";  // Handle empty fields with the string "empty"
            }

            // Move the start pointer past the comma
            start = (end != NULL) ? end + 1 : NULL;
        }

        // Ensure all 5 tokens are assigned, even if some are empty
        while (token_count < 5) {
            tokens[token_count++] = "empty";  // Handle any remaining empty fields
        }

        // Print how the request is read using tokens
        printf("Request read: Type = %s, Addr = %s, Data = %s, Fault = %s, Fault-Bit = %s\n", 
            tokens[0], tokens[1], tokens[2], tokens[3], tokens[4]);
        
        type = tokens[0][0];

        if (type == 'R' || type == 'W') {
                        
             //storing address if it exits othrwise error           
            if (strcmp(tokens[1], "empty") != 0) {
                // Form: R/W, x, , ,
                int base = 10;  // Default to decimal
                //here it accepts both decimal and hexademcimal as address
                if (tokens[1][0] == '0' && (tokens[1][1] == 'x' || tokens[1][1] == 'X')) {
                
                base = 16;  // Hexadecimal
                }
        	req.addr = (uint32_t)strtoul(tokens[1], NULL, base);
            } else {//if address empty retun error
                fprintf(stderr, "Error: Malformed  request: no address %s\n", line);  
                fclose(file);
                return -1;
            }

            //accepts fault &fault bit & error handling                
            if (token_count == 5 && strcmp(tokens[3], "empty") != 0  && strcmp(tokens[4], "empty") != 0) {
  
                // Form: R, x, , a, b
                int base = 10;//default decimal
                 if (tokens[3][0] == '0' && (tokens[3][1] == 'x' || tokens[3][1] == 'X')) {
                
                base = 16;  // Hexadecimal
                 }
                req.fault = (uint32_t)strtoul(tokens[3], NULL, base);
                if (tokens[4][0] == '0' && (tokens[4][1] == 'x' || tokens[4][1] == 'X')){
                fprintf(stderr, "Error: fault bit is given in Hex in the last treated request \n"); //  fault bit should not be in hex
                fclose(file);
                return -1;
                }
                req.faultBit = (uint8_t)strtoul(tokens[4], NULL, 10);//fault bit always as decimal
            
            } else if ((strcmp(tokens[3], "empty") == 0 && strcmp(tokens[4], "empty") != 0)||(strcmp(tokens[3], "empty") != 0 && strcmp(tokens[4], "empty") == 0)){
                //error when R,x,,a, or R,x,,,b
                fprintf(stderr, "Error: Malformed fault request: fault injection incomplete in %s\n", line);
                fclose(file);
                return -1;
            }

            if (type == 'R'){
                // data must be emtpy if not throw error
            if (strcmp(tokens[2], "empty") != 0 ) {
                fprintf(stderr, "Error: Malformed  Read request: Data in R not empty %s\n", line); 
                fclose(file);
                return -1;
                }

            req.r= 1; // Set write flag
            req.w = 0;

            } else {// type W
                

                //data must be a non emtpy entry
                if (strcmp(tokens[2], "empty") != 0 ) {

                 int base = 10;//default decimal
                 if (tokens[2][0] == '0' && (tokens[2][1] == 'x' || tokens[2][1] == 'X')) {
                    base = 16;  // Hexadecimal
                 }

                req.data = (uint32_t)strtoul(tokens[2], NULL, base);
                
                }else {//if data emty give error
                    fprintf(stderr, "Error: Malformed Write request: Data in W empty %s\n", line);
                    fclose(file);
                    return -1;
                }

                req.w = 1; // Set write flag
                req.r = 0;

            }

            } else if (type == 'F') {//  1 2 should be empty, 3 should be less than UINT32MAX  & 4 less than 8
            // Form: F, , , a, b
             if ((strcmp(tokens[1], "empty") != 0 || strcmp(tokens[2], "empty") != 0)||(strcmp(tokens[3], "empty") == 0  || strcmp(tokens[4], "empty") == 0) ){
                //error when F,,,a, or F,,,,b (faultBit or fault_address are empty)
                fprintf(stderr, "Error: Malformed fault request: data and address non empty or fault fields empty  %s\n", line);
                fclose(file);
                return -1;
            } else {
                int base = 10;//default decimal
                 if (tokens[3][0] == '0' && (tokens[3][1] == 'x' || tokens[3][1] == 'X')) {
                    base = 16;  // Hexadecimal
                 }
                req.fault = (uint32_t)strtoul(tokens[3], NULL, base);//accepts hex and dec
                 if (tokens[4][0] == '0' && (tokens[4][1] == 'x' || tokens[4][1] == 'X')){
                fprintf(stderr, "Error: fault bit is given in Hex in the last treated request \n"); //  fault bit should not be in hex
                fclose(file);
                return -1;
                }
                req.faultBit = (uint8_t)strtoul(tokens[4], NULL, 10);
            } 
        } else {
            fprintf(stderr, "Error: Invalid request type in line: %s\n", line);
            fclose(file);
            return -1;
        }

        
        requests[countReq++] = req;
        
    }

    fclose(file);
    return 0;
}


int main(int argc, char *argv[]) {
    
    // Default values for all options
    uint32_t cycles = 1000;             // Simulation cycles
    const char *tracefile = NULL;       // Trace file path (optional)
    const char *input_file = NULL;      // Input file (required)
    uint8_t endianness = 0;             // Endianness (default)
    uint32_t latencyScrambling = 1;   // Latency for address scrambling
    uint32_t latencyEncrypt = 3;      // Latency for encryption/decryption
    uint32_t latencyMemoryAccess = 4; // Latency for memory access
    uint32_t seed = 1234;                  // Seed for pRNG

    // Define CLI options
    static struct option long_options[] = {
        {"cycles", required_argument, 0, 'c'},
        {"tf", required_argument, 0, 't'},
        {"endianness", required_argument, 0, 'e'},
        {"latency-scrambling", required_argument, 0, 's'},
        {"latency-encrypt", required_argument, 0, 'l'},
        {"latency-memory-access", required_argument, 0, 'm'},
        {"seed", required_argument, 0, 'r'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    // Parse options using getopt_long
    int opt;
    while ((opt = getopt_long(argc, argv, "c:t:e:s:l:m:r:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                cycles = strtoul(optarg, NULL, 10);
                if (cycles <= 0 || cycles >= __UINT32_MAX__) {
                    fprintf(stderr, "Error: Number of cycles invalid.\n");
                    return EXIT_FAILURE;
                } 
                break;
            case 't':
                tracefile = optarg;
                break;
            case 'e':
                endianness = strtoul(optarg, NULL, 10);
                if (endianness > 1) {
                    fprintf(stderr, "Error: Invalid value for --endianness. Use 0 (little-endian) or 1 (big-endian).\n");
                return EXIT_FAILURE;
                }
                
                break;
                // latency scrambling should be at least 1 so the address scrambler module can finish scrambling 
            case 's':
                latencyScrambling = strtoul(optarg, NULL, 10);
                if (latencyScrambling < 1 || latencyScrambling >= __UINT32_MAX__) {
                    fprintf(stderr, "Error: Invalid value for --latencyScrambling. Must be at least 1 and less than UINT32_MAX\n");
                return EXIT_FAILURE;
                }
                break;
            case 'l':// latency encrypt should be at least 2 so the encrypter module can finish encrypting/decrypting 

                latencyEncrypt = strtoul(optarg, NULL, 10);
                if (latencyEncrypt < 2 || latencyEncrypt >= __UINT32_MAX__) {
                    fprintf(stderr, "Error: Invalid value for --latencyEncrypt. Must be at least 2 and less than UINT32_MAX\n");
                return EXIT_FAILURE;
                }
                break;
            case 'm':// latency memory should be at least 4 so the memory can store/access the data

                latencyMemoryAccess = strtoul(optarg, NULL, 10);
                if (latencyMemoryAccess < 4 || latencyMemoryAccess >= __UINT32_MAX__) {
                    fprintf(stderr, "Error: Invalid value for --latencyMemoryAccess. Must be at least 4 and less than UINT32_MAX\n");
                return EXIT_FAILURE;
                }
                break;
            case 'r':
                seed = strtoul(optarg, NULL, 10);
                if (seed >= __UINT32_MAX__ ) {
                    fprintf(stderr, "Error: Invalid value for --seed\n");
                return EXIT_FAILURE;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Handle positional argument (input file)
    if (optind < argc) {
        input_file = argv[optind];
    } else {
        fprintf(stderr, "Error: Input file is required.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    optind++;

    // Check for extra arguments beyond the input file
    if (optind < argc) {
        fprintf(stderr, "Error: Unexpected argument(s):");
        for (int i = optind; i < argc; i++) {
            fprintf(stderr, " %s", argv[i]);
        }
        fprintf(stderr, "\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Display parsed arguments
    printf("Parsed Arguments:\n");
    printf("  Cycles: %u\n", cycles);
    if (tracefile) {
        printf("  Tracefile: %s\n", tracefile);
    } else {
        printf("  Tracefile: None\n");
    }
    printf("  Endianness: %u\n", endianness);
    printf("  Latency Scrambling: %u\n", latencyScrambling);
    printf("  Latency Encrypt: %u\n", latencyEncrypt);
    printf("  Latency Memory Access: %u\n", latencyMemoryAccess);
    printf("  Seed: %u\n", seed);
    printf("  Input File: %s\n", input_file);

    uint32_t numRequests = count_requests(input_file);//calculate numRequessts

    struct Request *requests = (struct Request*) malloc(numRequests * sizeof(struct Request));//allocate place in memory for requests using the actual number of requests
    if (requests == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for requests.\n");
        return EXIT_FAILURE;
    }

    if (parse_requests(input_file, requests) != 0) { //parse the csv file for requests and store them in &requests
    free(requests); // Free allocated memory before exiting
    return EXIT_FAILURE;
}
    // Call the run_simulation function
    struct Result result = run_simulation(
    cycles,
    tracefile,
    endianness,
    latencyScrambling,
    latencyEncrypt,
    latencyMemoryAccess,
    seed,
    numRequests,  
    requests
    );

    //free the place for requests
    free(requests);


    // Display the simulation results
    printf("\nSimulation Results:\n");
    printf("  Total Cycles: %u\n", result.cycles);
    printf("  Errors Detected: %u\n", result.errors);
    printf("  Primitive Gate Count: %u\n", result.primitiveGateCount);

    return 0;
}
