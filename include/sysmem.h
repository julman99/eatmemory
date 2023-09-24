#include <stdio.h>

#ifdef __APPLE__
    #define SYSMEM_MODE_APPLE
#elif __linux__
    #define SYSMEM_MODE_LINUX
#else
    #define SYSMEM_MODE_NONE
#endif

size_t getTotalSystemMemory();
size_t getFreeSystemMemory();
long string_to_bytes(char * str);
char * bytes_to_string(long bytes, char * str); 