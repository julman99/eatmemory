#include <sysmem.h>
#ifdef SYSMEM_MODE_NONE

size_t getTotalSystemMemory(){
    return -1;
}

size_t getFreeSystemMemory(){
    return -1;
}

#endif