#include <sysmem.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>

const int TO_KB = 1024;
const int TO_MB = 1024 * TO_KB;
const int TO_GB = 1024 * TO_MB;

long string_to_bytes(char * str) {
    const size_t len = strlen(str);
    const char unit = str[len - 1];
    const char value_numeric[50] = "";
    
    strncpy(value_numeric, str, len);
    
    long number = atol(value_numeric);
    long bytes = number;
    if(!isdigit(unit) ) {
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
    if(bytes < 1024) {
        sprintf(str, "%ld bytes", bytes);
    } else if (bytes < 999 * TO_KB) {
        long kb = round(bytes / TO_KB);
        sprintf(str, "%ldK", kb);
    } else if (bytes < 999 * TO_MB) {
        long mb = round(bytes / TO_MB);
        sprintf(str, "%ldM", mb);
    } else {
        long gb = round(bytes / TO_GB);
        sprintf(str, "%ldG", gb);
    }
    return str;
}