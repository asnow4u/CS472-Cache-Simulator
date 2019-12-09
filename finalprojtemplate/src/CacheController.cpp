/*
	Cache Simulator (Starter Code) by Justin Goins
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
#include <vector>


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
    //this->globalWrites = 0;
    //this->globalReads = 0;


    srand(time(0));

   	// create your cache structure
    aiArray = new AddressInfo*[ci.numberSets]; 
    
    for (unsigned int i=0; i<ci.numberSets; i++){
        aiArray[i] = new AddressInfo[ci.associativity];
    }

    for (unsigned int i=0; i<ci.numberSets; i++){
        for (unsigned int j=0; j<ci.associativity; j++){
            aiArray[i][j].tag = 0;
            aiArray[i][j].setIndex = 0;
            aiArray[i][j].valid = 0;
            aiArray[i][j].LRUcounter = 0;
            aiArray[i][j].dirty = 0;
        }
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
    //Counters
    int numWrites, numReads;
    numWrites = 0;
    numReads = 0;
	// define regular expressions that are used to locate commands
	regex commentPattern("==.*");
	regex instructionPattern("I .*");
	regex loadPattern(" (L )(.*)(,[[:digit:]]+)$");
	regex storePattern(" (S )(.*)(,[[:digit:]]+)$");
	regex modifyPattern(" (M )(.*)(,[[:digit:]]+)$");

	// open the output file
	ofstream outfile(outputFile);
	// open the output file
	ifstream infile(inputFile);
	// parse each line of the file and look for commands
   
	while (getline(infile, line)) {
		// these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
        
		// create a struct to track cache responses
		CacheResponse response;
        response.hit = false;
        response.eviction = false;
        response.dirtyEviction = false;

		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;
		
        //Load Op
        } else if (std::regex_match(line, match, loadPattern)) {
			cout << "Found a load op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, false, address);
			updateCycles(&response, false);
            outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
            globalCycles += response.cycles;
	        numReads++;

        //Store Op
        } else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, true, address);
			updateCycles(&response, true);
            outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
            globalCycles += response.cycles;
            numWrites++;
		
        //Modify Op
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
			numReads++;
            response.hit = false;
            response.eviction = false;
            response.dirtyEviction = false;


            // now process the write operation
			cacheAccess(&response, true, address);
			updateCycles(&response, true);
            tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			totalCycles += response.cycles;
			outfile << " " << totalCycles << tmpString;
            globalCycles += totalCycles;
            numWrites++;

        //error    
		} else {
			throw runtime_error("Encountered unknown line format in tracefile.");
		}
		outfile << endl;
	}

    
    // add the final cache statistics
	outfile << "Hits: " << globalHits << " Misses: " << globalMisses << " Evictions: " << globalEvictions << endl;
	outfile << "Cycles: " << globalCycles << " Reads:" << numReads << " Writes:" << numWrites << endl;

	infile.close();
	outfile.close();

    //cache at the end
    /*
    cout << "State of Cache:" << endl;
    for (unsigned int i=0; i<ci.numberSets; i++){
        for (unsigned int j=0; j<ci.associativity; j++){
            cout << "aiArray[" << i << "][" << j << "] tag: " << aiArray[i][j].tag << " index: " << aiArray[i][j].setIndex << " valid: " << aiArray[i][j].valid << " LRU: " << aiArray[i][j].LRUcounter << endl;
        }
    }
     
    cout << "Hits: " << globalHits << " Misses: " << globalMisses << " Evictions: " << globalEvictions << endl;
	cout << "Cycles: " << globalCycles << " Reads:" << numReads << " Writes:" << numWrites << endl;
    */

    //Delete Cache
    for (unsigned int i = 0; i<ci.numberSets; i++){
        delete[] aiArray[i];
    }
    delete[] aiArray;
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	
    AddressInfo ai;
  	//int size = 0;
    //unsigned long int temp = address;
    //int binaryMask;

    //Find size of address
    //do {
    //    temp /= 2;
    //    size++;
    //} while (temp > 0);

    //Index bit
    //binaryMask = ~(~0 << (ci.numSetIndexBits + 1));
    //ai.setIndex = (address >> ci.numByteOffsetBits) & binaryMask;

    //Tag bit
    //binaryMask = ~(~0 << ((size - (ci.numSetIndexBits + ci.numByteOffsetBits)) + 1));
    //ai.tag = (address >> (ci.numSetIndexBits + ci.numByteOffsetBits) & binaryMask);

    ai.tag = address >> ci.numSetIndexBits >> ci.numByteOffsetBits;
    ai.setIndex = (address - (ai.tag << ci.numByteOffsetBits << ci.numSetIndexBits)) >> ci.numByteOffsetBits;


    //cout << "size Index: " << ci.numSetIndexBits << " offset: " << ci.numByteOffsetBits << endl; 
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

     //Search for matching tag within Index
     for (unsigned int i=0; i<ci.associativity; i++){
         
         if (aiArray[ai.setIndex][i].valid == 1){
            if (aiArray[ai.setIndex][i].tag == ai.tag){
                
                //Update LRUcounters 
                if (ci.rp == ReplacementPolicy::LRU) {
                    for (unsigned int j=0; j<ci.associativity; j++){
                        if (aiArray[ai.setIndex][j].valid == 1){
                            aiArray[ai.setIndex][j].LRUcounter++;
                        }
                    }

                    aiArray[ai.setIndex][i].LRUcounter = 0;
                }
               
                //Update Dirty Bit
                if (isWrite){
                    if (ci.wp == WritePolicy::WriteBack){
                        aiArray[ai.setIndex][i].dirty = 1;
                    }
                }

                response->hit = true;
            }
         }
     }

    if (!response->hit){
       
        bool foundEmpty = false;

        for (unsigned int i=0; i<ci.associativity; i++){

            //Check for empty block
            if (aiArray[ai.setIndex][i].valid == 0 && !foundEmpty){
                aiArray[ai.setIndex][i].valid = 1;
                aiArray[ai.setIndex][i].tag = ai.tag;
                aiArray[ai.setIndex][i].setIndex = ai.setIndex;
                foundEmpty = true;
            
            
                //Update LRUcounters 
                if(ci.rp == ReplacementPolicy::LRU) {
                    for (unsigned int j=0; j<ci.associativity; j++){
                        if (aiArray[ai.setIndex][j].valid == 1){
                            aiArray[ai.setIndex][j].LRUcounter++;
                        }
                    }

                    aiArray[ai.setIndex][i].LRUcounter = 0;
                }

                //Update Dirty Bit
                if (isWrite){
                    if (ci.wp == WritePolicy::WriteBack){
                        aiArray[ai.setIndex][i].dirty = 1;
                    }
                }
            }
        }

        //Use proper Replacement Policy
        if (!foundEmpty){
            
            //LRU
            if (ci.rp == ReplacementPolicy::LRU) {
                unsigned int leastUsed = 0;
                unsigned int leastUsedCounter = 0;
                
                //Find least used
                for (unsigned int i=0; i<ci.associativity; i++){
                    if (aiArray[ai.setIndex][i].LRUcounter > leastUsedCounter){
                        leastUsedCounter = aiArray[ai.setIndex][i].LRUcounter;
                        leastUsed = i;
                    }
                }

                aiArray[ai.setIndex][leastUsed].tag = ai.tag;
               
                //Update LRUcounters 
                for (unsigned int i=0; i<ci.associativity; i++){
                    if (aiArray[ai.setIndex][i].valid == 1){
                        aiArray[ai.setIndex][i].LRUcounter++;
                    }
                }

                aiArray[ai.setIndex][leastUsed].LRUcounter = 0;

                //Check Dirty Bit
                if (ci.wp == WritePolicy::WriteBack){
                    if (aiArray[ai.setIndex][leastUsed].dirty == 1){
                        response->dirtyEviction = true;  

                        //Reset dirty bit
                        if (!isWrite){
                            aiArray[ai.setIndex][leastUsed].dirty = 0;
                        }
                    } 
                }
             
                response->eviction = true;
            
                
            //Random
            } else {
                unsigned int num = rand() % ci.associativity; 
                aiArray[ai.setIndex][num].tag = ai.tag;
                
                //Check Dirty Bit
                if (ci.wp == WritePolicy::WriteBack){
                    if (aiArray[ai.setIndex][num].dirty == 1){
                        response->dirtyEviction = true;  

                        //Reset dirty bit
                        if (!isWrite){
                            aiArray[ai.setIndex][num].dirty = 0;
                        }
                    } 
                }
                
                response->eviction = true;
            }
        }
    }
    
    // your code needs to update the global counters that track the number of hits, misses, and evictions

	if (response->hit){
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
    if (isWrite == true) {
        
        //WriteThrough
        if (ci.wp == WritePolicy::WriteThrough){
            if (response->hit){
                response->cycles = ci.cacheAccessCycles + ci.memoryAccessCycles;
            } else {
                //fetch block and write to it + update cache and main memory
                response->cycles = ci.memoryAccessCycles *2 + ci.cacheAccessCycles*2;
            }

        //WriteBack
        } else if (ci.wp == WritePolicy::WriteBack) {
            
            //Check for diry bit
            if (response->dirtyEviction){
                response->cycles = 2*ci.cacheAccessCycles + 2*ci.memoryAccessCycles;
            } else {
                response->cycles = ci.cacheAccessCycles;
            }
        }

    //Load
    } else {
       
        if (response->hit){
            response->cycles = ci.cacheAccessCycles;
 
        } else {
            
            //Check for dirty bit
            if (response->dirtyEviction){
                response->cycles = ci.cacheAccessCycles + ci.memoryAccessCycles + ci.memoryAccessCycles + ci.memoryAccessCycles; //Access memory twice, once to write back to memory and to load from memory

            } else {
                response->cycles = ci.cacheAccessCycles + ci.memoryAccessCycles;
            }
        }
    }
}
