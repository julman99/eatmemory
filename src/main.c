/*
 * File:   eatmemory.c
 * Author: Julio Viera <julio.viera@gmail.com>
 *
 * Created on August 27, 2012, 2:23 PM
 */

#define VERSION "0.1.10"
#define _POSIX_C_SOURCE 1
#define STR_NA "N/A"
#define STR_CHUNK_AUTO "auto"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "eatmemory.h"
#include "args.h"
#include <unistd.h>
#include "errors.h"
#include "math.h"

char tmpstr[255] = "";
char tmpstr2[255] = "";

ArgParser* configure_cmd() {
    ArgParser* parser = ap_new_parser();
    ap_add_flag(parser, "help h ?");
    ap_add_int_opt(parser, "timeout t", -1);
    ap_add_str_opt(parser, "chunk-size s", STR_CHUNK_AUTO);
    return parser;
}

void print_help() {
    printf("eatmemory %s - %s\n\n", VERSION, "https://github.com/julman99/eatmemory");
    printf("Usage: eatmemory [-t <seconds>] <size>\n");
    printf("Size can be specified in megabytes or gigabytes in the following way:\n");
    printf("#                # Bytes      example: 1024\n");
    printf("#M               # Megabytes  example: 15M\n");
    printf("#G               # Gigabytes  example: 2G\n");
#ifndef SYSMEM_MODE_UNKOWN
    printf("#%%               # Percent    example: 50%%\n");
#endif
    printf("\n");
    printf("Options:\n");
    printf("-t <seconds>     Exit after specified number of seconds.\n");
    printf("-s <chunk_size>  Specify a custom chunk size in the same format\n");
    printf("                 as the memory to be eaten.\n");
    printf("                 Default: %s\n", STR_CHUNK_AUTO);
    printf("                 Max:     %s\n", bytes_to_string(SIZE_MAX, tmpstr));
    printf("\n");
}

void print_and_exit(char * error, eatmemory_error exit_code) {
    printf("ERROR %d: %s\n", exit_code, error);
    exit(exit_code);
}

void print_and_exit_if_error(eatmemory_error if_error, char * error_message, eatmemory_error exit_code) {
    if(if_error != EM_ERROR_NONE) {
        print_and_exit(error_message, exit_code);
    }
}

int main(int argc, char *argv[]){
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
    eatmemory_error err = 0;
    long size = string_to_bytes(memory_to_eat, &err);
    print_and_exit_if_error(err,"Memory to eat is invalid", EM_ERROR_MEMORY_ARG_INVALID);

    char * chunk_str = ap_get_str_value(parser, "chunk-size");
    long chunk = strcmp(chunk_str, STR_CHUNK_AUTO) != 0 ? string_to_bytes(chunk_str, &err) : get_auto_chunk_size(size);
    print_and_exit_if_error(err, "Chunk size is invalid", EM_ERROR_CHUNK_SIZE_ARG_INVALID);

    ap_free(parser);

    struct system_memory_stats memory_stats;
    get_system_memory_stats(&memory_stats);
    printf("Currently total memory:     %s\n", memory_stats.supported ? bytes_to_string(memory_stats.total, tmpstr) : STR_NA);
    printf("Currently available memory: %s\n", memory_stats.supported ? bytes_to_string(memory_stats.free, tmpstr) : STR_NA);
    printf("\n");
    
    // Check if process memory monitoring is supported and warn if not
    struct process_memory_stats process_test;
    get_process_memory_stats(&process_test);
    if (!process_test.supported) {
        printf("WARNING: Process memory usage monitoring is not supported on this operating system. We won't be able to automatically verify the memory consumption.\n");
        printf("\n");
    }
    
    printf("Eating %s in chunks of %s...\n", bytes_to_string(size, tmpstr), bytes_to_string(chunk, tmpstr2));
    
    eatmemory_error eat_error = EM_ERROR_NONE;
    int8_t** eaten = eat(size, chunk, &eat_error);
    
    if(eat_error == EM_ERROR_MEMORY_VERIFICATION_FAILED) {
        print_and_exit("Memory allocation verification failed - the process did not consume the expected amount of memory", EM_ERROR_MEMORY_VERIFICATION_FAILED);
    } else if(eat_error == EM_ERROR_CANNOT_ALLOCATE_MEMORY) {
        print_and_exit("Could not allocate the memory", EM_ERROR_CANNOT_ALLOCATE_MEMORY);
    }
    
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
    }
    digest(eaten, size, chunk);
}

