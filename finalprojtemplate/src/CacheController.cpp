/*
	Cache Simulator by Andrew Snow
    Starter Code by Justin Goins
	Oregon State University
	Fall Term 2019
*/

#include "CacheController.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <cmath>
#include <stdlib.h>
#include <time.h>
#include <array>

using namespace std;

CacheController::CacheController(CacheInfo ci, string tracefile) {
	// store the configuration info
	this->ci = ci;
	this->inputFile = tracefile;
	this->outputFile = this->inputFile + ".out";

	// compute the other cache parameters
	this->ci.numByteOffsetBits = log2(ci.blockSize);
	this->ci.numSetIndexBits = log2(ci.numberSets);

	// initialize the counters
	this->globalCycles = 0;
	this->globalHits = 0;
	this->globalMisses = 0;
	this->globalEvictions = 0;

	srand(time(0));
	// create your cache structure
    
    //Direct Mapped
    if (ci.associativity == 1){
        //Array of X elements
        AddressInfo *aiArray[ci.numberSets];
        bool accessArray[ci.numberSets];

        //Initilize Array
        for (int i=0; i<(int)ci.numberSets; i++){
            //aiArray[i] = nullptr;
            accessArray[i] = false;
        
        }
        
        
		aiArrayPointer = aiArray;
        accessed = accessArray;
        
        if (accessed[1]){
            cout << "test" << endl;
        }
      	
        directMapped = true;
		fullyAssociative = false;

    //Fully Associative
    } else if (ci.numberSets == 1){ //cache / (N * block)
        //Array of Y elements
        AddressInfo aiArray[ci.associativity];

				//Initilize Array
				for (int i=0; i<(int)ci.associativity; i++){
						aiArray[i].setIndex = 0;
						aiArray[i].tag = 0;
				}

				//aiArrayPointer = aiArray;
				directMapped = false;
				fullyAssociative = true;

    //Set Associative
    } else {
        //Array of X x Y elements
        AddressInfo aiArray[ci.numberSets][ci.associativity];

				for (int i=0; i<(int)ci.numberSets; i++){
					for (int j=0; j<(int)ci.associativity; j++){
							aiArray[i][j].setIndex = 0;
							aiArray[i][j].tag = 0;
					}
				}

				//aiArrayPointer = aiArray; //**aiSetArrayPointer;
				directMapped = false;
				fullyAssociative = false;
    }
}

/*
	Starts reading the tracefile and processing memory operations.
*/
void CacheController::runTracefile() {
	cout << "Input tracefile: " << inputFile << endl;
	cout << "Output file name: " << outputFile << endl;

	// process each input line
	string line;
	// define regular expressions that are used to locate commands
	regex commentPattern("==.*");
	regex instructionPattern("I .*");
	regex loadPattern(" (L )(.*)(,[[:digit:]]+)$");
	regex storePattern(" (S )(.*)(,[[:digit:]]+)$");
	regex modifyPattern(" (M )(.*)(,[[:digit:]]+)$");

	// open the output file
	ofstream outfile(outputFile);
	// open the input file
	ifstream infile(inputFile);

	// parse each line of the file and look for commands
	while (getline(infile, line)) {
		
        // these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
		// create a struct to track cache responses
		CacheResponse response;

		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;

        //Load OP
		} else if (std::regex_match(line, match, loadPattern)) {
		
            cout << "Found a load op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
            cacheAccess(&response, false, address);
			updateCycles(&response, false);
            outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
            globalCycles += response.cycles;

      //Store OP
		} else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, true, address);
			updateCycles(&response, true);
            outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
            globalCycles += response.cycles;

      //Modify OP
		} else if (std::regex_match(line, match, modifyPattern)) {
			cout << "Found a modify op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			
            // first process the read operation
			cacheAccess(&response, false, address);
            updateCycles(&response, false);

            string tmpString; // will be used during the file output
            tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			unsigned long int totalCycles = response.cycles; // track the number of cycles used for both stages of the modify operation
            
			// now process the write operation
			cacheAccess(&response, true, address);
            updateCycles(&response, true);
			tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			totalCycles += response.cycles;
			outfile << " " << totalCycles << tmpString;
            globalCycles += totalCycles;

      //Error
		} else {
			throw runtime_error("Encountered unknown line format in tracefile.");
		}

		outfile << endl;
	}
	// add the final cache statistics
	outfile << "Hits: " << globalHits << " Misses: " << globalMisses << " Evictions: " << globalEvictions << endl;
	outfile << "Cycles: " << globalCycles << endl;

	infile.close();
	outfile.close();
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {

	AddressInfo ai;
	int addressSize = 0;
	unsigned long int temp = address;
	int binaryMask;

    //Index bits
    binaryMask = ~(~0 << (ci.numSetIndexBits + 1));
    ai.setIndex = (address >> ci.numByteOffsetBits) & binaryMask;
   
    cout << "setIndex: " << ai.setIndex << endl;


    //Find size of address
    do {
       temp /= 2;
       addressSize++;
    } while (temp > 0);

    cout << "address bits: " << addressSize << endl; //test


    //Tag bits
    binaryMask = ~(~0 << ((addressSize - (ci.numSetIndexBits + ci.numByteOffsetBits)) + 1));
	ai.tag = (address >> (ci.numSetIndexBits + ci.numByteOffsetBits) & binaryMask);

    
	return ai;
}

/*
	This function allows us to read or write to the cache.
	The read or write is indicated by isWrite.
*/
void CacheController::cacheAccess(CacheResponse* response, bool isWrite, unsigned long int address) {

	// determine the index and tag
	AddressInfo ai = getAddressInfo(address);

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

	// your code needs to update the global counters that track the number of hits, misses, and evictions

	//directMapped
	if (directMapped){
       
        //Check to see if the index is empty
        if (accessed[ai.setIndex] == false){
            //Compare tags
            cout << "accessed" << endl;
			//if (aiArrayPointer[ai.setIndex]->tag == ai.tag){
			//	response->hit = true;
			//Replace what is written at the index
			//} else {
			//  aiArrayPointer[ai.setIndex] = &ai;
			//  response->eviction = true;
			//}
		//Nothing exists here
		} else {
            cout << "not accessed" << endl;
            //Replace what is written at the index (response->Miss)
			//aiArrayPointer[ai.setIndex] = &ai; 
		}
    
	//fullyAssociative
	} else if (fullyAssociative){
		//Loop through blocks
		for (int i=0; i<(int)sizeof(aiArrayPointer); i++){
			//Compare tags
			if (aiArrayPointer[i]->tag == ai.tag){
				response->hit = true;
			}
		}
		if (!response->hit){
			//Check for open block
			int count = 0;
			for (int i=0; i<(int)sizeof(aiArrayPointer); i++){
				if (aiArrayPointer[i]->tag == 0 && count < 1){
					//aiArrayPointer[i] = ai;
					count++;
				}
			}
			//Use the proper ReplacementPolicy
			if (count < 1){
				if (ci.rp == ReplacementPolicy::LRU){
					//LRU ReplacementPolicy
					//TODO update array
					response->eviction = true;
				} else {
					//Random ReplacementPolicy
					//aiArrayPointer[(rand() % (int)sizeof(aiArrayPointer))] = ai;
					response->eviction = true;
				}
			}
		}

	//setAssociative
	} else {
        //Loop through index looking for tag
        for (int i=0; i<(int)ci.associativity; i++){
           // if (aiSetArrayPointer[ai->setIndex][i].tag == ai.tag){
           //     response->hit = true;
           // }
        }

        if (!response->hit){
            //Search for empty block
           int count = 0;
           for (int i=0; i<(int)ci.associativity; i++){
               // if (aiSetArrayPointer[ai.setIndex][i].tag == 0 && count < 1){
                   // aiSetArrayPointer[ai.setIndex][i] = ai;
               //     count++;
               // }
           }
           //Use proper ReplacementPolicy
           if (count < 1){
                if (ci.rp == ReplacementPolicy::LRU){
                    //TODO update array
                    response->eviction = true;

                } else {
                   // aiSetArrayPointer[ai.setIndex][(rand() % (int)ci.associativity +1)] = ai;
                    response->eviction = true;

                }
           }
        }
	}

	if (response->hit) {
		globalHits++;
		cout << "Address " << std::hex << address << " was a hit." << endl;

	} else if (response->eviction){
		globalEvictions++;
		cout << "Address " << std::hex << address << " was an eviction." << endl;

	} else {
		globalMisses++;
		cout << "Address " << std::hex << address << " was a miss." << endl;
	}

	cout << "-----------------------------------------" << endl;

	return;
}

/*
	Compute the number of cycles used by a particular memory operation.
	This will depend on the cache write policy.
*/
void CacheController::updateCycles(CacheResponse* response, bool isWrite) {
	// your code should calculate the proper number of cycles
	//Store
	if (isWrite) {

		//WriteThrough Policy
		if (ci.wp == WritePolicy::WriteThrough){
			response->cycles = ci.cacheAccessCycles + ci.memoryAccessCycles;

		//WriteBack Policy
		} else if (ci.wp == WritePolicy::WriteBack){

			if (response->hit){
				response->cycles = ci.cacheAccessCycles;
			} else {
				response->cycles = ci.cacheAccessCycles + ci.memoryAccessCycles;
			}
        }

	//Load
	} else {

		if (response->hit){
			//Only accesses the cache
			response->cycles = ci.cacheAccessCycles;

		} else {
			//Missed: Access both cache and memory
			response->cycles = ci.cacheAccessCycles + ci.memoryAccessCycles;
		}
	}
}


