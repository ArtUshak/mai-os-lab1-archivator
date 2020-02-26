#ifndef PROGRAM_OPTIONS_H
#define PROGRAM_OPTIONS_H

#include <stdio.h>

// TODO: more flexible error and info reporting

enum program_mode
{
    MODE_PACK,   // create archive
    MODE_LIST,   // list directories and files in archive
    MODE_UNPACK, // extract archive
    MODE_HELP,   // print help message
    MODE_UNKNOWN // invalid mode or option
};

/* Program verbosity (which messages to display).
 */
enum program_verbosity
{
    VERBOSITY_QUIET,  // print error messages only
    VERBOSITY_VERBOSE // print error and information messages
};

#define FILE_CAT_DEFAULT_BUFFER_SIZE 4096

/* Program parameters (parsed from command line).
 */
struct program_parameters
{
    enum program_mode mode;
    enum program_verbosity verbosity;
    char* input_name;
    char* output_name;
    size_t file_cat_buffer_size;
};

/* Parse size input string. It can be in bytes (512), kilobytes (256K),
 * megabytes (128M). Return -1 on parsing error.
 */
ssize_t parse_size(const char* size_string);

/* Parse command line parameters and return them as structure.
 */
struct program_parameters parse_program_parameters(int argc,
                                                   char* const argv[]);

/* Print formatted error message and exit program with non-zero code.
 */
void print_error(const struct program_parameters* program_parameters,
                 const char* message,
                 ...);

/* Print error message from errno (using perror) and exit program with non-zero
 * code.
 */
void print_perror(const struct program_parameters* program_parameters,
                  const char* message);

/* Print formatted information message.
 */
void print_info(const struct program_parameters* program_parameters,
                const char* message,
                ...);

/* Print program usage help (program_name is executable name to display).
 */
void print_usage(const char* program_name);

#endif

