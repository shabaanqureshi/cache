/* Summer 2017 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../part1/utils.h"
#include "problem2.h"

/*
	Function used to return the result of the third cache parameters question.
*/
cache_t* params3() {
        uint32_t n;
        uint32_t blockDataSize;
        uint32_t totalDataSize;
        char* fileName;
        fileName = "testFiles/physicalMemory3.txt";
        /* Your Code Here. */
        totalDataSize = 512;
        blockDataSize = 64;
        n = 4;
        cache_t* cache = createCache(n,blockDataSize,totalDataSize,fileName);
        return cache;
}
