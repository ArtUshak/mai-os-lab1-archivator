#include "program_options.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
print_usage(const char* program_name)
{
    printf("Usage:\n");
    printf("%s MODE [OPTIONS] [INPUT] [OUTPUT]\n", program_name);
    printf("Modes:\n");
    printf(" pack                        create archive OUTPUT and add "
           "files from INPUT to it\n");
    printf(" list                        list files in archive INPUT\n");
    printf(" unpack                      extract archive INPUT to "
           "directory OUTPUT\n");
    printf(" help                        print this help message\n");
    printf("Options:\n");
    printf("   -h --help                 print this help message and exit\n");
    printf("   -v --verbose              print informational messages when "
           "running\n");
}

struct program_parameters
parse_program_parameters(int argc, char* const argv[])
{
    struct program_parameters program_parameters;
    program_parameters.mode = MODE_UNKNOWN;
    program_parameters.verbosity = VERBOSITY_QUIET;
    program_parameters.input_name = NULL;
    program_parameters.output_name = NULL;

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
        } else {
            // Parse INPUT and OUTPUT
            if ((program_parameters.mode == MODE_PACK) ||
                (program_parameters.mode == MODE_LIST) ||
                (program_parameters.mode == MODE_UNPACK)) {
                if (program_parameters.input_name == NULL) {
                    program_parameters.input_name = argument;
                    continue;
                }
                if (program_parameters.mode != MODE_LIST)
                    if (program_parameters.output_name == NULL) {

                        program_parameters.output_name = argument;
                        continue;
                    }
            }
        }

        // Unexpected argument
        fprintf(stderr, "Unexpected argument %s\n", argument);
        program_parameters.mode = MODE_UNKNOWN;
        break;
    }

    if ((program_parameters.mode == MODE_PACK) ||
        (program_parameters.mode == MODE_LIST) ||
        (program_parameters.mode == MODE_UNPACK)) {
        if (program_parameters.input_name == NULL) {
            fprintf(stderr, "INPUT is required, but was not given\n");
            program_parameters.mode = MODE_UNKNOWN;
        }
    }
    if ((program_parameters.mode == MODE_PACK) ||
        (program_parameters.mode == MODE_UNPACK)) {
        if (program_parameters.output_name == NULL) {
            fprintf(stderr, "OUTPUT is required, but was not given\n");
            program_parameters.mode = MODE_UNKNOWN;
        }
    }

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

