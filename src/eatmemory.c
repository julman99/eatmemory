#include <eatmemory.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

const int TO_KB = 1024;
const int TO_MB = 1024 * TO_KB;
const int TO_GB = 1024 * TO_MB;

long string_to_bytes(char * str) {
    const size_t len = strlen(str);
    char unit = str[len - 1];
    char value_numeric[50] = "";
    
    strncpy(value_numeric, str, len);
    
    long number = atol(value_numeric);
    long bytes = number;
    if(!isdigit(unit) ) {
        unit = toupper(unit);
        if(unit == 'K') {
            bytes = number * TO_KB;
        } else if(unit=='M') {
            bytes = number * TO_MB;
        } else if(unit=='G') {
            bytes = number * TO_GB;
        } else if (unit=='%') {
            bytes = bytes * ((long)getFreeSystemMemory())/100;
        }
    }
    
    return bytes;
}



char * bytes_to_string(long bytes, char * str){
    if(bytes < 0) {
        sprintf(str, "N/A");
    } else if (bytes < 1024) {
        sprintf(str, "%ld bytes", bytes);
    } else if (bytes < 1 * TO_MB -1) {
        long kb = round(bytes / TO_KB);
        sprintf(str, "%ldK", kb);
    } else if (bytes < 1 * TO_GB -1) {
        long mb = round(bytes / TO_MB);
        sprintf(str, "%ldM", mb);
    } else {
        long gb = round(bytes / TO_GB);
        sprintf(str, "%ldG", gb);
    }
    return str;
}


int8_t** eat(size_t total, size_t chunk) {
    unsigned long iterations = total/chunk;
    if(total % chunk > 0) {
        iterations++;
    }
    int8_t** allocations = malloc(sizeof(int8_t *) * iterations);
    memset(allocations, 0, sizeof(int8_t *) * iterations);

    size_t allocated = 0;
    for(unsigned long i=0; i<iterations; i++){
        size_t allocate = MIN(chunk, total - allocated);
        int8_t *buffer = malloc(sizeof(int8_t) * allocate);
        if(buffer == NULL){
            return NULL;
        }
        for(unsigned long j=0; j<sizeof(int8_t) * allocate; j++) {
            buffer[j] = 1;
        }
        allocations[i] = buffer;
        allocated += allocate;
    }
    return allocations;
}

void digest(int8_t** eaten, long total,int chunk) {
    unsigned long iterations = total/chunk;
    if(total % chunk > 0) {
        iterations++;
    }
    for(unsigned long i=0; i < iterations; i++){
        if(eaten[i] != NULL) {
            free(eaten[i]);
        }
    }
    free(eaten);
}
