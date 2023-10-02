#ifndef em_errors_h
#define em_errors_h

enum eatmemory_error {
    EM_ERROR_NONE = 0,
    EM_ERROR_MEMORY_ARG_INVALID = 10,
    EM_ERROR_CHUNK_SIZE_ARG_INVALID = 11,
    EM_ERROR_CANNOT_ALLOCATE_MEMORY = 20,
    EM_ERROR_PARSE_SYNTAX = 30,
    EM_ERROR_PARSE_OVERFLOW = 31
};

#define eatmemory_error enum eatmemory_error

#endif
