/*
 * File:   eatmemory.c
 * Author: Julio Viera <julio.viera@gmail.com>
 *
 * Created on August 27, 2012, 2:23 PM
 */

#define _POSIX_C_SOURCE 200809L
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

#ifndef SYSMEM_BACKEND
#define SYSMEM_BACKEND "Unkown OS"
#endif

ArgParser* configure_cmd(const struct eatmemory_backend* backend) {
    ArgParser* parser = ap_new_parser();
    ap_add_flag(parser, "help h ?");
    if (backend->memory_lock_supported) {
        ap_add_flag(parser, "lock-memory l");
    }
    ap_add_int_opt(parser, "timeout t", -1);
    ap_add_str_opt(parser, "chunk-size s", STR_CHUNK_AUTO);
    return parser;
}

void print_memory_summary(const struct eatmemory_backend* backend) {
    struct system_memory_stats memory_stats = {0};
    bool memory_stats_available = false;
    if (backend->memory_stats_supported) {
        memory_stats_available = eatmemory_get_system_memory_stats(&memory_stats) == EM_STATS_OK;
    }

    printf("Total memory:     %s\n", memory_stats_available ? bytes_to_string(memory_stats.total, BYTES_TMP_STR()) : STR_NA);
    printf("Available memory: %s\n", memory_stats_available ? bytes_to_string(memory_stats.free, BYTES_TMP_STR()) : STR_NA);
}

void print_help(const struct eatmemory_backend* backend) {
    printf("eatmemory %s - %s - %s\n\n", VERSION, "https://github.com/julman99/eatmemory", SYSMEM_BACKEND);
    printf("Usage: eatmemory [-t <seconds>] <size>\n");
    printf("Size can be specified in megabytes or gigabytes in the following way:\n");
    printf("#                 # Bytes      example: 1024\n");
    printf("#M                # Megabytes  example: 15M\n");
    printf("#G                # Gigabytes  example: 2G\n");
    if (backend->memory_stats_supported) {
        printf("#%%                # Percent    example: 50%%\n");
    }
    printf("\n");
    printf("Options:\n");
    printf("-t <seconds>      Exit after specified number of seconds.\n");
    printf("-s <chunk_size>   Specify a custom chunk size in the same format\n");
    printf("                  as the memory to be eaten.\n");
    printf("                  Default: %s\n", STR_CHUNK_AUTO);
    printf("                  Max:     %s\n", bytes_to_string(SIZE_MAX, BYTES_TMP_STR()));
    if (backend->memory_lock_supported) {
        printf("-l, --lock-memory Lock allocated pages in physical memory.\n");
    }
    printf("\n");
    print_memory_summary(backend);
}

void print_and_exit(char * error, eatmemory_error exit_code) {
    fprintf(stderr, "ERROR %d: %s\n", exit_code, error);
    exit(exit_code);
}

void print_and_exit_if_error(eatmemory_error if_error, char * error_message, eatmemory_error exit_code) {
    if(if_error != EM_ERROR_NONE) {
        print_and_exit(error_message, exit_code);
    }
}

int main(int argc, char *argv[]){
    struct eatmemory_backend backend;
    eatmemory_get_backend_capabilities(&backend);

    ArgParser* parser = configure_cmd(&backend);
    ap_parse(parser, argc, argv);
    if(ap_found(parser, "help")) {
        print_help(&backend);
        exit(0);
    }
    if(ap_count_args(parser) != 1) {
        print_help(&backend);
        exit(1);
    }

    int timeout = ap_get_int_value(parser, "timeout");

    char* memory_to_eat = ap_get_arg_at_index(parser, 0);
    eatmemory_error err = 0;
    size_t size = string_to_bytes(memory_to_eat, &err);
    print_and_exit_if_error(err,"Memory to eat is invalid", EM_ERROR_MEMORY_ARG_INVALID);

    char * chunk_str = ap_get_str_value(parser, "chunk-size");
    size_t chunk_size = strcmp(chunk_str, STR_CHUNK_AUTO) != 0 ? string_to_bytes(chunk_str, &err) : get_auto_chunk_size(size);
    print_and_exit_if_error(err, "Chunk size is invalid", EM_ERROR_CHUNK_SIZE_ARG_INVALID);
    bool lock_memory = false;
    if (backend.memory_lock_supported) {
        lock_memory = ap_found(parser, "lock-memory");
    }

    ap_free(parser);

    print_memory_summary(&backend);
    printf("\n");

    printf("Eating %s in chunks of %s...\n", bytes_to_string(size, BYTES_TMP_STR()), bytes_to_string(chunk_size, BYTES_TMP_STR()));

    eatmemory_error eat_error = EM_ERROR_NONE;
    struct allocation eaten = eat_with_options(size, chunk_size, lock_memory, &eat_error);

    if(eat_error == EM_ERROR_MEMORY_VERIFICATION_FAILED) {
        print_and_exit("Memory verification failed - a byte read back did not match the value written", EM_ERROR_MEMORY_VERIFICATION_FAILED);
    } else if(eat_error == EM_ERROR_CANNOT_ALLOCATE_MEMORY) {
        print_and_exit("Could not allocate the memory", EM_ERROR_CANNOT_ALLOCATE_MEMORY);
    } else if(eat_error == EM_ERROR_CHUNK_SIZE_ARG_INVALID) {
        print_and_exit("Chunk size must be greater than zero", EM_ERROR_CHUNK_SIZE_ARG_INVALID);
    } else if(eat_error == EM_ERROR_MEMORY_LOCK_UNSUPPORTED) {
        print_and_exit("Memory locking is not supported by this backend", EM_ERROR_MEMORY_LOCK_UNSUPPORTED);
    } else if(eat_error == EM_ERROR_CANNOT_LOCK_MEMORY) {
        print_and_exit("Could not lock the allocated memory; check OS locked-memory limits and permissions", EM_ERROR_CANNOT_LOCK_MEMORY);
    }

    if(eaten.chunks){
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
    digest(eaten);
}
