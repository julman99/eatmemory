// -----------------------------------------------------------------------------
// Args: a C99 library for parsing command line arguments.
// Version: 3.1.1
// -----------------------------------------------------------------------------

#ifndef args_h
#define args_h

#include <stdbool.h>

// -----------------------------------------------------------------------------
// Types.
// -----------------------------------------------------------------------------

// An ArgParser instance stores registered flags, options and commands.
typedef struct ArgParser ArgParser;

// A callback function should accept two arguments: the command's name and the
// command's ArgParser instance. It should return an integer status code.
typedef int (*ap_callback_t)(char* cmd_name, ArgParser* cmd_parser);

// -----------------------------------------------------------------------------
// Initialization, parsing, teardown.
// -----------------------------------------------------------------------------

// Allocates and initializes a new ArgParser instance. Returns NULL if memory
// allocation fails.
ArgParser* ap_new_parser();

// Specifies a helptext string for the parser. If [helptext] is not NULL, this
// activates an automatic --help/-h flag. (Either --help or -h can be overridden
// by explicitly registered flags.) The parser stores and manages its own copy
// of the [helptext] string so [helptext] can be freed immediately after this
// call if it was dynamically constructed.
void ap_set_helptext(ArgParser* parser, const char* helptext);

// Returns a pointer to the parser's helptext string.
char* ap_get_helptext(ArgParser* parser);

// Specifies a version string for the parser. If [version] is not NULL, this
// activates an automatic --version/-v flag. (Either --version or -v can be
// overridden by explicitly registered flags.) The parser stores and manages its
// own copy of the [version] string so [version] can be freed immediately after
// this call if it was dynamically constructed.
void ap_set_version(ArgParser* parser, const char* version);

// Returns a pointer to the parser's version string.
char* ap_get_version(ArgParser* parser);

// Parses an array of string arguments.
// - Exits with an error message and a non-zero status code if the arguments are
//   invalid.
// - The parameters are assumed to be [argc] and [argv] as supplied to main(),
//   i.e. the first element of the array is assumed to be the binary name and
//   is therefore ignored.
// - Returns true if the arguments were successfully parsed.
// - Returns false if parsing failed because sufficient memory could not be
//   allocated.
bool ap_parse(ArgParser* parser, int argc, char** argv);

// Frees the memory associated with the parser and any subparsers.
void ap_free(ArgParser* parser);

// -----------------------------------------------------------------------------
// Parsing modes.
// -----------------------------------------------------------------------------

// If set, the first positional argument ends option-parsing; all subsequent
// arguments will be treated as positionals.
void ap_first_pos_arg_ends_option_parsing(ArgParser* parser);

// -----------------------------------------------------------------------------
// Register flags and options.
// -----------------------------------------------------------------------------

// Registers a new flag.
void ap_add_flag(ArgParser* parser, const char* name);

// Registers a new string-valued option.
void ap_add_str_opt(ArgParser* parser, const char* name, const char* fallback);

// Registers a new integer-valued option.
void ap_add_int_opt(ArgParser* parser, const char* name, int fallback);

// Registers a new double-valued option.
void ap_add_dbl_opt(ArgParser* parser, const char* name, double fallback);

// Registers a new greedy string-valued option.
void ap_add_greedy_str_opt(ArgParser* parser, const char* name);

// -----------------------------------------------------------------------------
// Inspect flags and options.
// -----------------------------------------------------------------------------

// Returns the number of times the specified flag or option was found.
int ap_count(ArgParser* parser, const char* name);

// Returns true if the specified flag or option was found.
bool ap_found(ArgParser* parser, const char* name);

// Returns the value of a string option.
char* ap_get_str_value(ArgParser* parser, const char* name);

// Returns the string value at the specified index.
char* ap_get_str_value_at_index(ArgParser* parser, const char* name, int index);

// Returns the value of an integer option.
int ap_get_int_value(ArgParser* parser, const char* name);

// Returns the integer value at the specified index.
int ap_get_int_value_at_index(ArgParser* parser, const char* name, int index);

// Returns the value of a floating-point option.
double ap_get_dbl_value(ArgParser* parser, const char* name);

// Returns the floating-point value at the specified index.
double ap_get_dbl_value_at_index(ArgParser* parser, const char* name, int index);

// Returns an option's values as a freshly-allocated array of string
// pointers. The array's memory is not affected by calls to ap_free().
// Returns NULL if memory allocation fails.
char** ap_get_str_values(ArgParser* parser, const char* name);

// Returns an option's values as a freshly-allocated array of integers.
// The array's memory is not affected by calls to ap_free().
// Returns NULL if memory allocation fails.
int* ap_get_int_values(ArgParser* parser, const char* name);

// Returns an option's values as a freshly-allocated array of doubles.
// The array's memory is not affected by calls to ap_free().
// Returns NULL if memory allocation fails.
double* ap_get_dbl_values(ArgParser* parser, const char* name);

// -----------------------------------------------------------------------------
// Positional arguments.
// -----------------------------------------------------------------------------

// Returns true if the parser has found one or more positional arguments.
bool ap_has_args(ArgParser* parser);

// Returns the number of positional arguments.
int ap_count_args(ArgParser* parser);

// Returns the positional argument at the specified index.
char* ap_get_arg_at_index(ArgParser* parser, int index);

// Returns the positional arguments as a freshly-allocated array of string
// pointers. The memory occupied by the returned array is not affected by
// calls to ap_free(). Returns NULL if memory allocation fails.
char** ap_get_args(ArgParser* parser);

// Attempts to parse and return the positional arguments as a freshly allocated
// array of integers. Exits with an error message on failure. The memory
// occupied by the returned array is not affected by calls to ap_free().
// Returns NULL if memory allocation fails.
int* ap_get_args_as_ints(ArgParser* parser);

// Attempts to parse and return the positional arguments as a freshly allocated
// array of doubles. Exits with an error message on failure. The memory
// occupied by the returned array is not affected by calls to ap_free().
// Returns NULL if memory allocation fails.
double* ap_get_args_as_doubles(ArgParser* parser);

// -----------------------------------------------------------------------------
// Commands.
// -----------------------------------------------------------------------------

// Registers a new command. Returns the ArgParser instance for the command.
// Returns NULL if sufficient memory cannot be allocated for the new parser.
ArgParser* ap_new_cmd(ArgParser* parent_parser, const char* name);

// Registers a callback function on a command parser.
void ap_set_cmd_callback(ArgParser* cmd_parser, ap_callback_t cmd_callback);

// Returns true if [parent_parser] has found a command.
bool ap_found_cmd(ArgParser* parent_parser);

// If [parent_parser] has found a command, returns its name, otherwise NULL.
char* ap_get_cmd_name(ArgParser* parent_parser);

// If [parent_parser] has found a command, returns its ArgParser instance,
// otherwise NULL.
ArgParser* ap_get_cmd_parser(ArgParser* parent_parser);

// If [parent_parser] has found a command, and if that command has a callback
// function, returns the exit code from the callback, otherwise 0.
int ap_get_cmd_exit_code(ArgParser* parent_parser);

// This boolean switch toggles support for an automatic 'help' command that
// prints subcommand helptext. The value defaults to false but gets toggled
// automatically to true whenever a command is registered. You can use this
// function to disable the feature if required.
void ap_enable_help_command(ArgParser* parent_parser, bool enable);

// If [parser] has a parent -- i.e. if [parser] is a command parser -- returns
// its parent, otherwise NULL.
ArgParser* ap_get_parent(ArgParser* parser);

// -----------------------------------------------------------------------------
// Debugging utilities.
// -----------------------------------------------------------------------------

// Dump a parser instance for debugging.
void ap_print(ArgParser* parser);

// Returns true if an attempt to allocate memory failed.
bool ap_had_memory_error(ArgParser* parser);

#endif
