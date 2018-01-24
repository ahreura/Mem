#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "sfmm.h"

#define EINVAL 22
#define ENOMEM 12
#define HF (SF_HEADER_SIZE + SF_FOOTER_SIZE)/8

int findIndex (int blockSize);
void* coalesceBackward(void* ptr);
void* coalesceForward(void* ptr);
void locateFreeBlock(void* Header);
void* findFreeBlock(int listIndex, int blockSize);
void* allocateBlock(void* freeBlock, int blockSize, int size);
void deleteBlock(void* deletedBlock, int blockSize);