#include <stdio.h>

#ifdef __APPLE__
    #define SYSMEM_MODE_APPLE
#elif defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGE_SIZE)
    #define SYSMEM_MODE_LINUX
#else
    #define SYSMEM_MODE_NONE
#endif

//system memory stats
size_t getTotalSystemMemory();
size_t getFreeSystemMemory();

//mem string parsing
long string_to_bytes(char * str);
char * bytes_to_string(long bytes, char * str);

//mem allocation
int8_t** eat(size_t total, size_t chunk);
void digest(int8_t** eaten, long total,int chunk);