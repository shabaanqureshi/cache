/* Summer 2017 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "utils.h"
#include "setInCache.h"
#include "getFromCache.h"
#include "cacheWrite.h"

/*
	Takes in a cache and block number and value (either 1 or 0) and sets
	the valid bit at that block number to the value given.
*/
void setValid(cache_t* cache, uint32_t blockNumber, uint8_t value) {
	uint64_t validBit = getValidLocation(cache, blockNumber);
	setBit(cache,validBit,value);
}

/*
	Takes in a cache and block number and value (either 1 or 0) and sets
	the dirty bit at that block number to the value given.
*/
void setDirty(cache_t* cache, uint32_t blockNumber, uint8_t value) {
	uint64_t dirtyBit = getDirtyLocation(cache, blockNumber);
	setBit(cache,dirtyBit,value);
}

/*
	Takes in a cache and block number and value (either 1 or 0) and sets
	the shared bit at that block number to the value given.
*/
void setShared(cache_t* cache, uint32_t blockNumber, uint8_t value) {
	uint64_t sharedBit = getSharedLocation(cache, blockNumber);
	setBit(cache,sharedBit,value);

}

/*
	Takes in a cache, a location, and a value (either 0 or 1) and sets the bit
	at that bit location in the cache to the value passed in.
*/
void setBit(cache_t* cache, uint64_t location, uint8_t value) {
	uint64_t byteLoc = location >> 3;
	uint64_t shiftAmount = location & 7;
	value = (value << (7 - shiftAmount));
	cache->contents[byteLoc] = (cache->contents[byteLoc] & ~( 1 << (7-shiftAmount)));
	cache->contents[byteLoc] = (cache->contents[byteLoc] | value);

}

/*
	Takes in a cache, a block number, and an LRU value and sets the LRU for
	that block number to the LRU value passed in.
*/
void setLRU(cache_t* cache, uint32_t blockNumber, long newLRU) {
	uint64_t location = getLRULocation(cache,blockNumber);
	uint8_t LRU = numLRUBits(cache);
	for (int i = 0; i < LRU; i++){
		uint8_t singleBitValue = (uint8_t)  newLRU >> ((LRU - 1)- i);
		singleBitValue = singleBitValue & 1;
		setBit(cache,location + i,singleBitValue);
	}
}

/* 
	Takes in a cache, a pointer to data, a block number, a length or the data,
	and an offset value. Sets the data in the block given by the block number
	at the offset specified.
*/
void setData(cache_t* cache, uint8_t* data, uint32_t blockNumber, uint32_t length, uint32_t offset) {
	uint8_t temp;
	uint8_t mask;
	uint32_t start;
	uint64_t totalBits;
	uint64_t location = getDataLocation(cache, blockNumber, offset);
	uint64_t byteLoc = location >> 3;
	int shiftAmount = location & 7;
	if (shiftAmount == 0) {
		for (uint32_t i = 0; i < length; i++) {
			cache->contents[byteLoc + i] = data[i];
		}
	} else {
		start = 0;
		mask = 0;
		for (int j = 0; j < (8 - shiftAmount); j++) {
			mask += (1 << j);
		}
		mask = ~mask;
		totalBits = length << 3;
		temp = data[start] >> (shiftAmount);
		cache->contents[byteLoc + start] = cache->contents[byteLoc + start] & mask;
		cache->contents[byteLoc + start] = cache->contents[byteLoc + start] | temp;
		totalBits -= (8 - shiftAmount);
		while (totalBits > 7) {
			temp = data[start] << (8 - shiftAmount);
			temp = temp | (data[start + 1] >> shiftAmount);
			cache->contents[byteLoc + start + 1] = temp;
			totalBits -= 8;
			start += 1;
		}
		temp = data[start] << (8 - shiftAmount);
		mask = 0;
		for (int k = 0; k < shiftAmount; k++) {
			mask += (1 << (7 - k));
		}
		mask = ~mask;
		cache->contents[byteLoc + start + 1] = cache->contents[byteLoc + start + 1] & mask;
		cache->contents[byteLoc + start + 1] = cache->contents[byteLoc + start + 1] | temp;
	}
}

/*
	Takes in a cache, tag, and block numbers sets the tag for the block 
	specified to be the value passed in.
*/
void setTag(cache_t* cache, uint32_t tag, uint32_t blockNumber) {
	uint8_t mask;
	uint8_t temp;
	uint64_t location = getTagLocation(cache, blockNumber);
	uint64_t byteLoc = location >> 3;
	uint8_t shiftAmount = location & 7;
	uint8_t totalBits = getTagSize(cache);
	int start = 0;
	mask = 0;
	if (totalBits + shiftAmount < 8) {
		for (int i = shiftAmount; i < totalBits + shiftAmount; i++) {
			mask += 1 << (7 - i);
		}
		cache->contents[byteLoc + start] = cache->contents[byteLoc + start] & ~mask;
		temp = (uint8_t) (tag << (8 - shiftAmount - totalBits));
		temp = temp & mask;
		cache->contents[byteLoc + start] = cache->contents[byteLoc] | temp;
		return;
	} else {
		for (int i = 0; i < (8 - shiftAmount); i++) {
			mask += (1 << i);
		}
		cache->contents[byteLoc + start] = cache->contents[byteLoc + start] & ~mask;
		temp = (uint8_t) (tag >> (shiftAmount + totalBits - 8));
		temp = temp & mask;
		cache->contents[byteLoc + start] = cache->contents[byteLoc] | temp;
		totalBits -= (8 - shiftAmount);
		start += 1;
	}
	while (totalBits > 7) {
		temp = (uint8_t) (tag >> (totalBits - 8));
		temp = temp & UINT8_MAX;
		cache->contents[byteLoc + start] = temp;
		start += 1;
		totalBits -= 8;
	}
	if (totalBits != 0) {
		mask = 0;
		for (int j = 0; j < totalBits; j++) {
			mask += (1 << (7 - j));
		}
		cache->contents[byteLoc + start] = cache->contents[byteLoc + start] & ~mask;
		temp = (uint8_t) (tag << (8 - totalBits));
		temp = temp & mask;
		cache->contents[byteLoc + start] = cache->contents[byteLoc + start] | temp;
	}
}

/*
	Takes a newly initialized cache or a cache which has shifted programs and
	sets all of the valid bits to 0. Also sets all LRU bits to the maximum value. 
	Effectively clears the cache.
*/
void clearCache(cache_t* cache) {
	/* Your Code Here. */
	int numBlocks = (cache->totalDataSize)/(cache->blockDataSize);
	uint8_t LRUBits = numLRUBits(cache);
	
	for(int i = 0; i < numBlocks; i++){
		setValid(cache,i,0);
		if (LRUBits > 0)		
			setLRU(cache,i,0xff);
	}
	cache->hit = 0.0;
	cache->access = 0.0;
}

/*
	Takes in a cache that is switching between programs and clears it, writing
	an dirty values to memory.
*/
void contextSwitch(cache_t* cache) {
	uint32_t numBlocks = getNumSets(cache) << (log_2(cache->n));
	for (int i = 0; i < numBlocks; i++) {
		evict(cache, i);
	}
	clearCache(cache);
}

/*
	Takes in a newly created cache or cleared cache and initialises all LRU 
	values to be maximal.
*/
void initializeLRU(cache_t* cache) {
	for (int i = 0; i < getNumSets(cache) << log_2(cache->n); i++) {
		setLRU(cache, i, cache->n - 1);
	}
}

/*
	Takes in a cache, the tag, and index of the accessed data
	along with the original LRU way and updates all of the LRU's in the
	cache that need to be updated.
*/
void updateLRU(cache_t* cache, uint32_t tag, uint32_t idx, long oldLRU) {
	long currLRU;
	uint32_t blockNumber;
	uint32_t blockNumberStart = idx << log_2(cache->n);
	for (int i = 0; i < cache->n; i++) {
		blockNumber = blockNumberStart + i;
		if (tagEquals(blockNumber, tag, cache) && getValid(cache, blockNumber)) {
			setLRU(cache, blockNumber, 0);
		} else {
			currLRU = getLRU(cache, blockNumber);
			if (currLRU < oldLRU) {
				setLRU(cache, blockNumber, currLRU + 1);
			}
		}
	}
}
