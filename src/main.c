#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "file_wrapper.h"
#include "listdir.h"
#include "program_options.h"

struct program_parameters
parse_program_parameters(int argc, char* argv[])
{
    struct program_parameters program_parameters;
    program_parameters.mode = MODE_UNKNOWN;
    program_parameters.verbosity = VERBOSITY_QUIET;
    program_parameters.input_name = NULL;
    program_parameters.output_name = NULL;

    int i;
    for (i = 1; i < argc; i++) {
        char* argument = argv[i];

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
			if (program_parameters.output_name == NULL)
			{
		   
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

int
main(int argc, char* argv[])
{
    struct program_parameters program_parameters =
      parse_program_parameters(argc, argv);

    switch (program_parameters.mode) {
        case MODE_PACK: {
            char* root_paths[2];
            root_paths[0] = program_parameters.input_name;
            root_paths[1] = NULL;

            struct file_data* input_directory_data =
              list_directory(root_paths, &program_parameters);

            archive_ptr_t current_position = sizeof(struct archive_header);
            assign_archive_positions(
              input_directory_data, &current_position, &program_parameters);
            assign_archive_content_positions(
              input_directory_data, &current_position, &program_parameters);

            struct file_wrapper* output_file = file_creat(
              program_parameters.output_name, S_IRUSR | S_IWUSR | S_IRGRP);
            if (output_file == NULL) {
                print_perror(&program_parameters, "file_creat() failed");
            }

            write_full_archive(
              input_directory_data, output_file, &program_parameters);

            if (file_close(output_file) < 0) {
                print_perror(&program_parameters, "file_close() failed");
            }

            free_directory_tree(input_directory_data);

            break;
        }
        case MODE_LIST: {
            struct file_wrapper* input_file =
              file_open(program_parameters.input_name, O_RDONLY);
            if (input_file == NULL) {
                print_perror(&program_parameters, "file_open() failed");
            }

            struct file_data* input_archive_data =
              read_full_archive(input_file, &program_parameters);

            if (file_close(input_file) < 0) {
                print_perror(&program_parameters, "file_close() failed");
            }

            print_directory_tree(input_archive_data, 0);

            free_directory_tree(input_archive_data);

            break;
        }
        case MODE_UNPACK: {
            struct file_wrapper* input_file =
              file_open(program_parameters.input_name, O_RDONLY);
            if (input_file == NULL) {
                print_perror(&program_parameters, "file_open() failed");
            }

            struct file_data* input_archive_data =
              read_full_archive(input_file, &program_parameters);

            read_archive_content(input_archive_data,
                                 input_file,
                                 program_parameters.output_name,
                                 &program_parameters);

            if (file_close(input_file) < 0) {
                print_perror(&program_parameters, "file_close() failed");
            }

            free_directory_tree(input_archive_data);
            break;
        }
        default: {
            print_usage(argv[0]);
            // TODO!
            break;
        }
    }

    return 0;
}

