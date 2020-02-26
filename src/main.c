#include <fcntl.h>

#include "archive.h"
#include "file_wrapper.h"
#include "listdir.h"
#include "program_options.h"

int
main(int argc, char* argv[])
{
    const struct program_parameters program_parameters =
      parse_program_parameters(argc, argv);

    switch (program_parameters.mode) {
        case MODE_PACK: {
            char* root_paths[2];
            root_paths[0] = program_parameters.input_name;
            root_paths[1] = NULL;

            struct file_data* const input_directory_data =
              list_directory(root_paths, &program_parameters);

            archive_ptr_t current_position = sizeof(struct archive_header);
            assign_archive_positions(
              input_directory_data, &current_position, &program_parameters);
            assign_archive_content_positions(
              input_directory_data, &current_position, &program_parameters);

            struct file_wrapper* const output_file = file_creat(
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
            struct file_wrapper* const input_file =
              file_open(program_parameters.input_name, O_RDONLY);
            if (input_file == NULL) {
                print_perror(&program_parameters, "file_open() failed");
            }

            struct file_data* const input_archive_data =
              read_full_archive(input_file, &program_parameters);

            if (file_close(input_file) < 0) {
                print_perror(&program_parameters, "file_close() failed");
            }

            print_directory_tree(input_archive_data, 0);

            free_directory_tree(input_archive_data);

            break;
        }
        case MODE_UNPACK: {
            struct file_wrapper* const input_file =
              file_open(program_parameters.input_name, O_RDONLY);
            if (input_file == NULL) {
                print_perror(&program_parameters, "file_open() failed");
            }

            struct file_data* const input_archive_data =
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

            break;
        }
    }

    return 0;
}

