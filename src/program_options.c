#include "program_options.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

