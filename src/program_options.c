#include "program_options.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
print_usage(const char* program_name)
{
    printf("Usage:\n");
    printf("%s MODE [OPTIONS]\n", program_name);
    printf("Modes:\n");
    printf(" pack                        create archive OUTPUT and add\n"
           "                             files from INPUT to it\n");
    printf(" list                        list files in archive INPUT\n");
    printf(" unpack                      extract archive INPUT to\n"
           "                             directory OUTPUT\n");
    printf(" help                        print this help message\n");
    printf("Options:\n");
    printf("   -h --help                 print this help message and exit\n");
    printf("   -v --verbose              print informational messages when\n"
           "                             running\n");
    printf("   -i --input NAME           input file or directory name\n");
    printf("   -o --output NAME          output file or directory name\n");
    printf(
      "      --buffer-size SIZE     use given buffer size for archive\n"
      "                             file reading and writing, can be\n"
      "                             given in bytes (like 512), kilobytes\n"
      "                             (like 256K) and megabytes (like 128M)\n");
    printf(
      "      --use-symlinks         add symlinks to created archive file\n");
    printf("      --ignore-symlinks      ignore symlinks\n");
}

ssize_t
parse_size(const char* size_string)
{
    ssize_t result = 0;
    const char* ptr;
    for (ptr = size_string; *ptr; ptr++) {
        result *= 10;
        if (isdigit(*ptr)) {
            result += (ssize_t)(*ptr - '0');
            continue;
        }
        if (*ptr == 'K') {
            result /= 10;
            result *= (1 << 10);
            ptr++;
            break;
        }
        if (*ptr == 'M') {
            result /= 10;
            result *= (1 << 20);
            ptr++;
            break;
        }
    }
    if (*ptr)
        return -1;
    return result;
}

struct program_parameters
parse_program_parameters(int argc, char* const argv[])
{
    struct program_parameters program_parameters;
    program_parameters.mode = MODE_UNKNOWN;
    program_parameters.verbosity = VERBOSITY_QUIET;
    program_parameters.input_name = NULL;
    program_parameters.output_name = NULL;
    program_parameters.file_cat_buffer_size = FILE_CAT_DEFAULT_BUFFER_SIZE;
    program_parameters.symlink_mode = SYMLINK_MODE_UNKNOWN;

    int i;
    for (i = 1; i < argc; i++) {
        char* const argument = argv[i];

        // Parse options
        if ((strcmp(argument, "--help") == 0) ||
            (strcmp(argument, "-h") == 0)) {
            program_parameters.mode = MODE_HELP;
            break;
        }
        if ((strcmp(argument, "--verbose") == 0) ||
            (strcmp(argument, "-v") == 0)) {
            program_parameters.verbosity = VERBOSITY_VERBOSE;
            continue;
        }
        if ((strcmp(argument, "--input") == 0) ||
            (strcmp(argument, "-i") == 0)) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "Error: Option --input requires path\n");
                program_parameters.mode = MODE_UNKNOWN;
                break;
            } else {
                i++;
                program_parameters.input_name = argv[i];
            }
            continue;
        }
        if ((strcmp(argument, "--output") == 0) ||
            (strcmp(argument, "-o") == 0)) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "Error: Option --output requires path\n");
                program_parameters.mode = MODE_UNKNOWN;
                break;
            } else {
                i++;
                program_parameters.output_name = argv[i];
            }
            continue;
        }
        if (strcmp(argument, "--buffer-size") == 0) {
            if ((i + 1) >= argc) {
                fprintf(stderr, "Error: Option --buffer-size requires size\n");
                program_parameters.mode = MODE_UNKNOWN;
                break;
            } else {
                i++;
                ssize_t size = parse_size(argv[i]);
                if (size < 0) {
                    fprintf(stderr, "Error: Invalid size value %s\n", argv[i]);
                    program_parameters.mode = MODE_UNKNOWN;
                    break;
                }
                program_parameters.file_cat_buffer_size = (size_t)size;
                continue;
            }
        }
        if (strcmp(argument, "--ignore-symlinks") == 0) {
            program_parameters.symlink_mode = SYMLINK_MODE_IGNORE;
            continue;
        }
        if (strcmp(argument, "--use-symlinks") == 0) {
            program_parameters.symlink_mode = SYMLINK_MODE_PHYSICAL;
            continue;
        }

        if (program_parameters.mode == MODE_UNKNOWN) {
            // Parse modes
            if (strcmp(argument, "pack") == 0) {
                program_parameters.mode = MODE_PACK;
                continue;
            }
            if (strcmp(argument, "list") == 0) {
                program_parameters.mode = MODE_LIST;
                continue;
            }
            if (strcmp(argument, "unpack") == 0) {
                program_parameters.mode = MODE_UNPACK;
                continue;
            }
            if (strcmp(argument, "help") == 0) {
                program_parameters.mode = MODE_HELP;
                break;
            }
        }

        // Unexpected argument
        fprintf(stderr, "Error: Unexpected argument %s\n", argument);
        program_parameters.mode = MODE_UNKNOWN;
        break;
    }

    if ((program_parameters.mode == MODE_PACK) ||
        (program_parameters.mode == MODE_LIST) ||
        (program_parameters.mode == MODE_UNPACK)) {
        if (program_parameters.input_name == NULL) {
            fprintf(stderr, "Error: INPUT is required, but was not given\n");
            program_parameters.mode = MODE_UNKNOWN;
        }
    }
    if ((program_parameters.mode == MODE_PACK) ||
        (program_parameters.mode == MODE_UNPACK)) {
        if (program_parameters.output_name == NULL) {
            fprintf(stderr, "Error: OUTPUT is required, but was not given\n");
            program_parameters.mode = MODE_UNKNOWN;
        }
    }

    if (program_parameters.symlink_mode == SYMLINK_MODE_UNKNOWN)
        program_parameters.symlink_mode = SYMLINK_MODE_PHYSICAL;

    return program_parameters;
}

void
print_error(__attribute__((unused))
            const struct program_parameters* program_parameters,
            const char* message,
            ...)
{
    va_list argptr;
    va_start(argptr, message);
    vfprintf(stderr, message, argptr);
    va_end(argptr);
    exit(-1);
}

void
print_perror(__attribute__((unused))
             const struct program_parameters* program_parameters,
             const char* message)
{
    perror(message);
    exit(-1);
}

void
print_info(const struct program_parameters* program_parameters,
           const char* message,
           ...)
{
    if (program_parameters->verbosity != VERBOSITY_VERBOSE)
        return;
    va_list argptr;
    va_start(argptr, message);
    vfprintf(stderr, message, argptr);
    va_end(argptr);
}

