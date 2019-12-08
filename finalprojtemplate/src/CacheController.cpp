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

		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;
		} else if (std::regex_match(line, match, loadPattern)) {
			cout << "Found a load op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, false, address);
			outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
		} else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, true, address);
			outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
		} else if (std::regex_match(line, match, modifyPattern)) {
			cout << "Found a modify op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			// first process the read operation
			cacheAccess(&response, false, address);
			string tmpString; // will be used during the file output
			tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			unsigned long int totalCycles = response.cycles; // track the number of cycles used for both stages of the modify operation
			// now process the write operation
			cacheAccess(&response, true, address);
			tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			totalCycles += response.cycles;
			outfile << " " << totalCycles << tmpString;
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
  	int size = 0;
    unsigned long int temp = address;
    int binaryMask;

    //Find size of address
    do {
        temp /= 2;
        size++;
    } while (temp > 0);

    //Index bit
    binaryMask = ~(~0 << (ci.numSetIndexBits + 1));
    ai.setIndex = (address >> ci.numByteOffsetBits) & binaryMask;

    //Tag bit
    binaryMask = ~(~0 << ((size - (ci.numSetIndexBits + ci.numByteOffsetBits)) + 1));
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

	if (response->hit)
		cout << "Address " << std::hex << address << " was a hit." << endl;
	else
		cout << "Address " << std::hex << address << " was a miss." << endl;

	cout << "-----------------------------------------" << endl;

	return;
}

/*
	Compute the number of cycles used by a particular memory operation.
	This will depend on the cache write policy.
*/
void CacheController::updateCycles(CacheResponse* response, bool isWrite) {
	// your code should calculate the proper number of cycles
	response->cycles = 0;
}
