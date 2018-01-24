
#include "functionCalls.h"
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};
int sbrkCalled = 0;
int pageSize = 0;
int maxSize = 0;
int sf_errno = 0;
int freeBlockSize = 0;
// #define PAGE_SZ 4096

//still have to take care of the situation for splitting between pages
void *sf_malloc(size_t size){
// check the size
    if(size == 0){
        sf_errno = EINVAL;
        return NULL;
    }
    else if(size > (PAGE_SZ *4)){ // 4pages - 16(header and footer)
        sf_errno = EINVAL;
        return NULL;
    }
// create the block for the allocated block
    sf_header *newHeader;
    int listIndex;
    int blockSize = 16 + size; // header&footer + memsize
    if(blockSize % 16 != 0){
            blockSize += 16-(blockSize % 16);
    }
    listIndex = findIndex(blockSize);

    // don't forget to keep track of sbrkCalled and maxSize!!!!
    sf_header *freeBlockHeader = (sf_header*) findFreeBlock(listIndex, blockSize);
    sf_free_header *freeBlock = (sf_free_header*) freeBlockHeader;
    // if you need a new page
    if(freeBlock == NULL){
        while(maxSize < blockSize){
            sbrkCalled++;
            if(sbrkCalled >=5){
                sf_errno = ENOMEM;
                return NULL;
            }
            newHeader = sf_sbrk();
            maxSize += PAGE_SZ;
           // printf("%p\n", newHeader);
            int tempBlockSize = PAGE_SZ/16;
            newHeader->block_size = tempBlockSize;
            newHeader->allocated = 0;
            if(sbrkCalled == 1){
                sf_footer *newFooter;
                newFooter = get_heap_end()-8;
                newFooter->block_size = newHeader->block_size;
                locateFreeBlock(newHeader);
            }else{
                newHeader = coalesceBackward(newHeader);
            }
        }
        // allocate the wanted block from the free block
        newHeader = allocateBlock(newHeader, blockSize, size);
        newHeader->allocated = 1;
        //sf_blockprint(newHeader);
        maxSize -= blockSize;
        // sf_snapshot();

        void* rAddress = newHeader;
        return rAddress+8;
    }
    else{
        newHeader = allocateBlock(freeBlockHeader, blockSize, size);
        newHeader->allocated = 1;
        maxSize -= blockSize;
        // sf_snapshot();
       void* rAddress = newHeader;
        return rAddress+8;
    }

     return NULL;
}
void* allocateBlock(void* freeBlock, int blockSize, int size){

    sf_header *newHeader = (sf_header *) freeBlock;
    sf_header *nFreeBlock = (sf_header *) freeBlock;

    sf_footer *newFooter;
    void *address = newHeader;
    int freeBlockSize = nFreeBlock -> block_size * 16;
    newHeader->block_size = blockSize;
    deleteBlock(freeBlock, freeBlockSize);
    if(freeBlockSize-blockSize < 32){
        newHeader->allocated = 1;
        newHeader->block_size = freeBlockSize/16;
        newHeader->unused = blockSize-size - 16;
        newFooter = address + freeBlockSize - 8;
        newFooter->block_size = newHeader->block_size;
        newFooter->allocated = newHeader->allocated;
        newFooter->requested_size = size;
    }
    else{
        newHeader->allocated = 1;
        newHeader->block_size = blockSize/16;
        newHeader->unused = blockSize-size - 16;
        newFooter = address + blockSize - 8;
        newFooter->block_size = newHeader->block_size;
        newFooter->allocated = newHeader->allocated;
        newFooter->requested_size = size;
        // make a new free block and locate them get rid off the current one
        address = newFooter;
        sf_header *newFreeHeader = address+8;
        newFreeHeader->block_size = (freeBlockSize-(blockSize))/16;
        newFreeHeader->allocated = 0;
        newFreeHeader->unused = 0;
        sf_footer *newFreeFooter;
        address = newFreeHeader;
        newFreeFooter = address + freeBlockSize-blockSize - 8;
        newFreeFooter->block_size = newFreeHeader->block_size;
        newFreeFooter->allocated = newFreeHeader->allocated;
        locateFreeBlock(newFreeHeader);
    }
    if(newFooter->requested_size + 16 != newHeader->block_size){
        newHeader->padded = 1;
        newFooter->padded = newHeader->padded;
    }
    // sf_blockprint(newHeader);
    return newHeader;
}
void deleteBlock(void* delBlock, int blockSize){
    // just deleting the footer and the header and dislocating inside of the list
    int f = 0;
    int index = findIndex(blockSize);

    sf_free_header *temp = seg_free_list[index].head;
   // sf_free_header *dblock = (sf_free_header*) delBlock;
    while(temp != NULL && f == 0){
        if(temp == delBlock){
            if(temp == seg_free_list[index].head){
                int min = seg_free_list[index].min;
                int max = seg_free_list[index].max;
                if(temp->next != NULL){
                    free_list nFreeList = {temp->next, min,max};
                    seg_free_list[index] = nFreeList;
                }
                else{
                    free_list nFreeList = {NULL, min,max};
                    seg_free_list[index] = nFreeList;
                }
            }
            else{
                if(temp->next != NULL){
                    sf_free_header *next = temp->next;
                    next->prev = temp->prev;
                }
                else temp->next = 0;
                if(temp->prev != NULL){
                    sf_free_header *prev = temp->prev;
                    prev->next = temp->next;
                }
            }
            f = 1;
        }
        temp = temp->next;
    }
}
void* findFreeBlock(int listIndex, int blockSize){
    int index = listIndex;
    while(index < 4){
        sf_free_header *block;
        while(seg_free_list[index].head == NULL){
            if(index<4){
                index++;
            }
            if(index >= 4)
                return NULL;

        }
        block = seg_free_list[index].head;
        if(block != NULL && block->next == NULL){
            if((block->header).block_size *16>= blockSize)
                return block;
            else
                return NULL;
        }
        while(block->next != NULL){
            if((block->header).block_size*16 >= blockSize){
                return block;
            }
            block = block->next;
        }
        index++;
    }
    return NULL;
}
void locateFreeBlock(void* Header){
    sf_header *header = (sf_header*) Header;
    int index = findIndex(header->block_size *16);
    if(seg_free_list[index].head == NULL){
        sf_free_header *tempHead= (sf_free_header*) header;
        tempHead->header = *header;
        free_list freeList = {tempHead, seg_free_list[index].min, seg_free_list[index].max};
        seg_free_list[index] = freeList;
    }
    else{
        sf_free_header *tempHead= seg_free_list[index].head;
        sf_free_header *newHead = (sf_free_header*) header;
        newHead->header = *header;
        newHead->next = tempHead;
        newHead->prev = tempHead->prev;
        tempHead->prev = newHead;
        free_list freeList = {newHead, seg_free_list[index].min, seg_free_list[index].max};
        seg_free_list[index] = freeList;
    }
    // sf_snapshot();
}
// getting the header of the free block that is just made by sbrk
void* coalesceBackward(void* header){
    // have to search trough the freelist and find the freeblock witht he right address
    void* address = header - 8;
    sf_header * Header = header;
    sf_footer *c = address;
    int blockSize = c->block_size * 16;
    int totalBlockSize = Header->block_size * 16 + blockSize;
    // find the adjacent block
    int index = 0;
    sf_free_header *temp;
    while(index < 4){
        while(seg_free_list[index].head == NULL){
            if(index<4){
                index++;
            }
            if(index >= 4)
                return header;
        }
        if(index >= 4)
            return header;

        temp = seg_free_list[index].head;
        if(temp != NULL && temp->next == NULL){
            if(temp == address-blockSize + 8){
                deleteBlock(address-blockSize + 8, blockSize);
                sf_header* newHeader = address - blockSize + 8;
                newHeader->block_size = totalBlockSize/16;
                newHeader->allocated = 0;
                sf_footer* newFooter = address + totalBlockSize -8;
                newFooter->block_size = newHeader->block_size;
                newFooter->allocated = 0;
                return newHeader;
            }
            else
                return header;
        }
        while(temp->next != NULL){
            if(temp == address-blockSize + 8){
                deleteBlock(address-blockSize + 8, blockSize);
                sf_header* newHeader = address - blockSize + 8;
                newHeader->block_size = totalBlockSize/16;
                newHeader->allocated = 0;
                sf_footer* newFooter = address + totalBlockSize -8;
                newFooter->block_size = newHeader->block_size;
                newFooter->allocated = 0;
                return newHeader;
            }
            temp = temp->next;
        }
        index++;
    }
    return header;
}
void* coalesceForward(void* header){
    // have to search trough the freelist and find the freeblock witht he right address
    sf_header * Header = header;
    int blockSize = Header->block_size * 16;
    void* t = header + blockSize;
    sf_header* cHeader = header + blockSize;
    int cBlockSize = cHeader->block_size * 16;
    int index = 0;
    sf_free_header *temp;
    while(index<4){
        while(seg_free_list[index].head == NULL){
            if(index<4){
                index++;
            }
            if(index >= 4)
                return header;
        }
        temp = seg_free_list[index].head;
        if(temp != NULL && temp->next == NULL){
            if(temp == t){
                deleteBlock(cHeader, cBlockSize);
                Header->block_size = (blockSize + cBlockSize)/16;
                sf_footer* footer = header + blockSize + cBlockSize - 8;
                footer->block_size = Header->block_size;
                footer->allocated = 0;
                return header;
            }
        }
        while(temp->next != NULL){
            if(temp == t){
                deleteBlock(cHeader, cBlockSize);
                Header->block_size = (blockSize + cBlockSize)/16;
                sf_footer* footer = header+ blockSize + cBlockSize - 8;
                footer->block_size = Header->block_size;
                footer->allocated = 0;
                return header;
            }
              temp = temp->next;
        }
        index++;
    }
    return header;
}
int findIndex (int blockSize){
    int listIndex;
            //LIST 1 _MIN 32 MAX 128
        if(blockSize >= LIST_1_MIN && blockSize <= LIST_1_MAX){
            listIndex = 0;
        }
        // LIST 2 MIN 129_MAX 512
        else if(blockSize>= LIST_2_MIN && blockSize <= LIST_2_MAX){
            listIndex = 1;
        }
        // LIST_3_MIN 513 MAX 2048
        else if(blockSize>= LIST_3_MIN && blockSize <= LIST_3_MAX){
            listIndex = 2;
        }
        // LIST_4_MIN 2049_MAX -1
        else if(blockSize>= LIST_4_MIN){
            listIndex = 3;
        }
    return listIndex;
}
void *sf_realloc(void *ptr, size_t size) {
    if(ptr == NULL){
        sf_errno = EINVAL;
        return NULL;
    }
    else if(size == 0){
        sf_free(ptr);
        return NULL;
    }
    int originalSize = size;
    sf_header *header = ptr - 8;

    int blockSize = header->block_size *16;
    if(header->allocated == 0){
        sf_errno = EINVAL;
        return NULL;
    }
    size += 16;
    if(size % 16 != 0){
            size += 16-(size % 16);
    }
    // realloc to larger size
    if(blockSize < size){
        void* p = sf_malloc(originalSize);
        if(p != NULL){
            memcpy(p, ptr, blockSize);
            sf_free(ptr);
        }
        return p;
    }
    //realloc to smaller size
    else if(blockSize > size){
        if(blockSize-size < 32){
            header->unused += blockSize-size;
            void* address  = header;
            sf_footer *footer = address + blockSize- 8;
            footer->requested_size = originalSize;
            header->allocated = 1;
            footer->allocated = header->allocated;
            header->padded = 1;
            footer->padded = header->padded;
            header->block_size = blockSize/16;
            footer->block_size = header->block_size;
            return address + 8;
        }
        else if(blockSize-size >=32){
            // make new freeblock
            void* address = header;
            sf_header * newFreeHeader = address + size;
            void* aaa = newFreeHeader;
            newFreeHeader->allocated = 0;
            newFreeHeader->block_size = (blockSize - size)/16;
            sf_footer *newFreeFooter = aaa - 8;
            newFreeFooter->allocated = newFreeHeader->allocated;
            newFreeFooter->block_size = newFreeHeader->block_size;
            newFreeFooter->requested_size = originalSize;
            // coelesce
            newFreeHeader = coalesceForward(newFreeHeader);
            // locate to the right place
            locateFreeBlock(newFreeHeader);
            header->block_size = size/16;
            header->allocated = 1;
            sf_footer * footer = address + size - 8;
            footer->allocated = header->allocated;
            footer->block_size = header->block_size;
            // sf_snapshot();
            void* rAddress = header;
            return rAddress +8;
            }

        // header = allocateBlock(header, size);

    }
	return NULL;
}

void sf_free(void *ptr) {
    sf_header *header = ptr -8;
    int blockSize = header->block_size * 16;
    if(header->allocated == 0){
        abort();
    }
        header->allocated = 0;
        header->padded = 0;
        void* address = header;
        sf_footer *footer = address + blockSize - 8;
        footer->allocated = 0;
        footer->padded = 0;
        header = coalesceForward(header);
        locateFreeBlock(header);
        //sf_snapshot();
}
