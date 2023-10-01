#include <stdio.h>
#include <stdint.h>

#ifdef __APPLE__
    #define SYSMEM_MODE_APPLE
#elif defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGE_SIZE)
    #define SYSMEM_MODE_LINUX
#else
    #define SYSMEM_MODE_UNKOWN
#endif

//system memory stats
size_t getTotalSystemMemory();
size_t getFreeSystemMemory();

//mem string parsing
size_t string_to_bytes(char * str);
char * bytes_to_string(size_t bytes, char * str);

//mem allocation
int8_t** eat(size_t total, size_t chunk);
void digest(int8_t** eaten, size_t total, size_t chunk);