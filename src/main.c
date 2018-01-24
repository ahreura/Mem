#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

    sf_mem_init();


    //double* ptr = sf_malloc(3900);
    void* p = sf_malloc(30);
    void* x = sf_malloc(30);

   // sf_blockprint(p-8);
 //    p = sf_malloc(3900);
    //*ptr = 320320320-320;

    sf_free(x);
    sf_free(p);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
