/*
 * File:   eatmemory.c
 * Author: Julio Viera <julio.viera@gmail.com>
 *
 * Created on August 27, 2012, 2:23 PM
 */

#define VERSION "0.1.9"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include "args/args.h"

#if defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PAGE_SIZE)
#define MEMORY_PERCENTAGE
#endif

#ifdef MEMORY_PERCENTAGE
size_t getTotalSystemMemory(){
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

size_t getFreeSystemMemory(){
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif

ArgParser* configure_cmd() {
    ArgParser* parser = ap_new_parser();
    ap_add_flag(parser, "help h ?");
    ap_add_int_opt(parser, "timeout t", -1);
    return parser;
}

void print_help() {
    printf("eatmemory %s - %s\n\n", VERSION, "https://github.com/julman99/eatmemory");
    printf("Usage: eatmemory [-t <seconds>] <size>\n");
    printf("Size can be specified in megabytes or gigabytes in the following way:\n");
    printf("#             # Bytes      example: 1024\n");
    printf("#M            # Megabytes  example: 15M\n");
    printf("#G            # Gigabytes  example: 2G\n");
#ifdef MEMORY_PERCENTAGE
    printf("#%%           # Percent    example: 50%%\n");
#endif
    printf("\n");
    printf("Options:\n");
    printf("-t <seconds>  Exit after specified number of seconds\n");
    printf("\n");
}

short** eat(long total,int chunk){
	long i;
    short** allocations = malloc(sizeof(short*) * (total/chunk));
	for(i=0;i<total;i+=chunk){
		short *buffer=malloc(sizeof(char)*chunk);
        if(buffer==NULL){
            return NULL;
        }
		memset(buffer,0,chunk);
        allocations[i/chunk] = buffer;
	}
    return allocations;
}

void digest(short** eaten, long total,int chunk) {
    long i;
    for(i=0;i<total;i+=chunk){
        free(eaten[i/chunk]);
    }
}

int main(int argc, char *argv[]){

#ifdef MEMORY_PERCENTAGE
    printf("Currently total memory: %zd\n",getTotalSystemMemory());
    printf("Currently avail memory: %zd\n",getFreeSystemMemory());
#endif

    ArgParser* parser = configure_cmd();
    ap_parse(parser, argc, argv);
    if(ap_found(parser, "help")) {
        print_help();
        exit(0);
    }
    if(ap_count_args(parser) != 1) {
        print_help();
        exit(1);
    }

    int timeout = ap_get_int_value(parser, "timeout");
    char* memory_to_eat = ap_get_args(parser)[0];

    ap_free(parser);

    int len=strlen(memory_to_eat);
    char unit=memory_to_eat[len - 1];
    long size=-1;
    int chunk=1024;
    if(!isdigit(unit) ){
        if(unit=='M' || unit=='G'){
            memory_to_eat[len-1]=0;
            size=atol(memory_to_eat) * (unit=='M'?1024*1024:1024*1024*1024);
        }
#ifdef MEMORY_PERCENTAGE
        else if (unit=='%') {
            size = (atol(memory_to_eat) * (long)getFreeSystemMemory())/100;
        }
#endif
        else{
            printf("Invalid size format\n");
            exit(0);
        }
    }else{
        size=atoi(memory_to_eat);
    }
    if(size <=0 ) {
        printf("ERROR: Size must be a positive integer");
        exit(1);
    }
    printf("Eating %ld bytes in chunks of %d...\n",size,chunk);
    short** eaten = eat(size,chunk);
    if(eaten){
        if(timeout < 0 && isatty(fileno(stdin))) {
            printf("Done, press ENTER to free the memory\n");
            getchar();
        } else if (timeout >= 0) {
            printf("Done, sleeping for %d seconds before exiting...\n", timeout);
            sleep(timeout);
        } else {
            printf("Done, kill this process to free the memory\n");
            while(true) {
                sleep(1);
            }
        }
        digest(eaten, size, chunk);
    }else{
        printf("ERROR: Could not allocate the memory");
    }

}

