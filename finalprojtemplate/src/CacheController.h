/*
	Cache Simulator (Starter Code) by Justin Goins
	Oregon State University
	Fall Term 2019
*/

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include "CacheStuff.h"
#include <string>

class CacheController {
	private:
		struct AddressInfo {
			unsigned long int tag;
			unsigned int setIndex;
            unsigned int valid;
            unsigned int LRUcounter;
            unsigned int dirty;
		};
		unsigned int globalCycles;
		unsigned int globalHits;
		unsigned int globalMisses;
		unsigned int globalEvictions;
        
        //unsigned int globalWrites;
        //unsigned int globalReads;

		std::string inputFile, outputFile;
      
        AddressInfo** aiArray;

		CacheInfo ci;

		// function to allow read or write access to the cache
		void cacheAccess(CacheResponse*, bool, unsigned long int);
		// function that can compute the index and tag matching a specific address
		AddressInfo getAddressInfo(unsigned long int);
		// compute the number of clock cycles used to complete a memory access
		void updateCycles(CacheResponse*, bool);

	public:
		CacheController(CacheInfo, std::string);
		void runTracefile();
};

#endif //CACHECONTROLLER
