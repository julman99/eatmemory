/*
 * File:   eatmemory.c
 * Author: Julio Viera <julio.viera@gmail.com>
 *
 * Created on August 27, 2012, 2:23 PM
 */

#define VERSION "0.1.10"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sysmem.h>
#include <args.h>
#include <unistd.h>
#include <errors.h>

char tmpstr[255] = "";
char tmpstr2[255] = "";

ArgParser* configure_cmd() {
    ArgParser* parser = ap_new_parser();
    ap_add_flag(parser, "help h ?");
    ap_add_int_opt(parser, "timeout t", -1);
    ap_add_str_opt(parser, "chunk c", "1K");
    return parser;
}

void print_help() {
    printf("eatmemory %s - %s\n\n", VERSION, "https://github.com/julman99/eatmemory");
    printf("Usage: eatmemory [-t <seconds>] <size>\n");
    printf("Size can be specified in megabytes or gigabytes in the following way:\n");
    printf("#                # Bytes      example: 1024\n");
    printf("#M               # Megabytes  example: 15M\n");
    printf("#G               # Gigabytes  example: 2G\n");
#ifdef MEMORY_PERCENTAGE
    printf("#%%             # Percent    example: 50%%\n");
#endif
    printf("\n");
    printf("Options:\n");
    printf("-t <seconds>     Exit after specified number of seconds.\n");
    printf("-c <chunk_size>  Specify a custom chunk size in the same format\n");
    printf("                 as the memory to be eaten. Defaults to 1024 bytes.\n");
    printf("\n");
}

void print_error(char * error, int exit_code) {
    printf("ERROR: %s\n", error);
    exit(exit_code);
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
    
    long size = string_to_bytes(memory_to_eat);
    char * chunk_str = ap_get_str_value(parser, "chunk");
    long chunk = string_to_bytes(chunk_str);
    
    if(size < 0 ) {
        print_error("Memory to eat is invalid", ERROR_MEMORY_ARG_INVALID);
    }

    ap_free(parser);

    printf("Currently total memory:     %s\n", bytes_to_string(getTotalSystemMemory(), tmpstr));
    printf("Currently available memory: %s\n", bytes_to_string(getFreeSystemMemory(), tmpstr));
    printf("\n");
    printf("Eating %s in chunks of %s...\n", bytes_to_string(size, tmpstr), bytes_to_string(chunk, tmpstr2));
    int8_t** eaten = eat(size, chunk);
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
    }else{
        print_error("Could not allocate the memory", ERROR_CANNOT_ALLOCATE_MEMORY);
    }
    digest(eaten, size, chunk);
}

