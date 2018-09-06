/* Summer 2017 */
#include <stdbool.h>
#include <stdint.h>
#include "utils.h"
#include "setInCache.h"
#include "cacheRead.h"
#include "cacheWrite.h"
#include "getFromCache.h"
#include "mem.h"
#include "../part2/hitRate.h"

/*
	Takes in a cache and a block number and fetches that block of data, 
	returning it in a uint8_t* pointer.
*/
uint8_t* fetchBlock(cache_t* cache, uint32_t blockNumber) {
	uint64_t location = getDataLocation(cache, blockNumber, 0);
	uint32_t length = cache->blockDataSize;
	uint8_t* data = malloc(sizeof(uint8_t) << log_2(length));
	if (data == NULL) {
		allocationFailed();
	}
	int shiftAmount = location & 7;
	uint64_t byteLoc = location >> 3;
	if (shiftAmount == 0) {
		for (uint32_t i = 0; i < length; i++) {
			data[i] = cache->contents[byteLoc + i];
		}
	} else {
		length = length << 3;
		data[0] = cache->contents[byteLoc] << shiftAmount;
		length -= (8 - shiftAmount);
		int displacement = 1;
		while (length > 7) {
			data[displacement - 1] = data[displacement - 1] | (cache->contents[byteLoc + displacement] >> (8 - shiftAmount));
			data[displacement] = cache->contents[byteLoc + displacement] << shiftAmount;
			displacement++;
			length -= 8;
		}
		data[displacement - 1] = data[displacement - 1] | (cache->contents[byteLoc + displacement] >> (8 - shiftAmount));
	}
	return data;
}

/*
	Takes in a cache, an address, and a dataSize and reads from the cache at
	that address the number of bytes indicated by the size. If the data block 
	is already in the cache it retrieves the contents. If the contents are not
	in the cache it is read into a new slot and if necessary something is 
	evicted.
*/
uint8_t* readFromCache(cache_t* cache, uint32_t address, uint32_t dataSize) {
	uint32_t offset = getOffset(cache, address);

	uint32_t index  = getIndex(cache, address);
	uint32_t tag    = getTag(cache, address);
	evictionInfo_t* info = findEviction(cache, address);
	uint8_t* data;
	if (info->match == 0) {
		evict(cache, info->blockNumber);
		data = readFromMem(cache, address - offset);
/*		writeToCache(cache,address - offset, data,dataSize);*/
		setData(cache,data,info->blockNumber,cache->blockDataSize,0);
		setValid(cache, info->blockNumber, 1);
		setTag(cache,tag,info->blockNumber);
		setDirty(cache,info->blockNumber,0);
		free(data);
	} else {
		reportHit( cache);

	}
    data = getData(cache, offset, info->blockNumber, dataSize);
	updateLRU(cache, tag, index, info->LRU);
	reportAccess(cache);
	free(info);
	return data;





/*
	-check if valid
	-update LRU
	-if not in cache need to kick the LRU and ?set dirty?, and read from mem and put in cache and update.
	-check that address valid and that tag are equal from the data and from the address.
*/

	/*uint8_t* d =readFromMem(cache, address); -gonna need to call this function if the contents not in the cache*/
}
/*
	Takes in a cache and an address and fetches a byte of data.
	Returns a struct containing a bool field of whether or not
	data was successfully read and the data. This field should be
	false only if there is an alignment error or there is an invalid
	address selected.
*/
byteInfo_t readByte(cache_t* cache, uint32_t address) {
	byteInfo_t retVal;
	/* Your Code Here. */
	if (!validAddresses(address,1) || cache == NULL){ 
		retVal.success = false;
	    	return retVal;
    }



	uint8_t* data = readFromCache(cache,address,1);
	retVal.data = *data;
	retVal.success = true;
	free(data);
	return retVal;
}

/*
	Takes in a cache and an address and fetches a halfword of data.
	Returns a struct containing a bool field of whether or not
	data was successfully read and the data. This field should be
	false only if there is an alignment error or there is an invalid
	address selected.
*/
halfWordInfo_t readHalfWord(cache_t* cache, uint32_t address) {
	halfWordInfo_t retVal;
	byteInfo_t temp;
	/* Your Code Here. */
    if (!validAddresses(address,2) || (address % 2 != 0) || cache == NULL){     	
    	retVal.success = false;
    	return retVal;
    }
	if (cache->blockDataSize < 2) {
		temp = readByte(cache, address);
		retVal.data = temp.data;
		temp = readByte(cache, address + 1);
		retVal.data = (retVal.data << 8) | temp.data;
		return retVal;
	}


	retVal.success = true;
	uint8_t* data = readFromCache(cache,address,2);
	retVal.data = ((uint16_t) data[0] << 8) | ((uint16_t) data[1]);
	free(data);
	return retVal;
}

/*
	Takes in a cache and an address and fetches a word of data.
	Returns a struct containing a bool field of whether or not
	data was successfully read and the data. This field should be
	false only if there is an alignment error or there is an invalid
	address selected.
*/
wordInfo_t readWord(cache_t* cache, uint32_t address) {
	wordInfo_t retVal;
	halfWordInfo_t temp;
	/* Your Code Here. */
    if (!validAddresses(address,4) || (address % 4 != 0) || cache == NULL){ 
    		retVal.success = false;
    		return retVal;
    }

	if (cache->blockDataSize < 4) {
		temp = readHalfWord(cache, address);
		retVal.data = temp.data;
		temp = readHalfWord(cache, address + 2);
		retVal.data = (retVal.data << 16) | temp.data;
		return retVal;
	}

	retVal.success = true;
	uint8_t* data = readFromCache(cache,address,4);
	retVal.data = ((uint32_t) data[0] << 24) + ((uint32_t) data[1] << 16) + ((uint32_t) data[2] << 8) + ((uint32_t) data[3]);
	free(data);
	return retVal;
}

/*
	Takes in a cache and an address and fetches a double word of data.
	Returns a struct containing a bool field of whether or not
	data was successfully read and the data. This field should be
	false only if there is an alignment error or there is an invalid
	address selected.
*/
doubleWordInfo_t readDoubleWord(cache_t* cache, uint32_t address) {
	doubleWordInfo_t retVal;
	wordInfo_t temp;
	/* Your Code Here. */
    if (!validAddresses(address,8) || (address % 8 != 0) || cache == NULL){
       	retVal.success = false;   
    	return retVal;
    }

	if (cache->blockDataSize < 8) {
		temp =  readWord(cache, address);
		retVal.data = temp.data;
		temp =  readWord(cache, address + 4);
		retVal.data = (retVal.data << 32) | temp.data;
		return retVal;
	}

	retVal.success = true;
	uint8_t* data = readFromCache(cache,address,8);
	retVal.data = (((uint64_t) data[0]) << 56) | (((uint64_t) data[1]) << 48) | (((uint64_t) data[2]) << 40) | (((uint64_t) data[3]) << 32)
	| (((uint64_t) data[4]) << 24) | (((uint64_t) data[5]) << 16) | (((uint64_t) data[6]) << 8) | data[7];
	free(data);
	return retVal;
}
