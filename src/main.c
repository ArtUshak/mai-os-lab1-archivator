#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "filewrapper.h"
#include "listdir.h"

// TODO: more flexible error and info reporting

enum program_mode {
	MODE_PACK,
	MODE_LIST,
	MODE_UNPACK,
	MODE_HELP,
	MODE_UNKNOWN
};

struct program_options {
	enum program_mode mode;
	char *input_name;
	char *output_name;
};

void print_usage(const char *program_name)
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
	printf(
	    "   -h --help                 print this help message and exit\n");
}

/* Parse command line options and return them as structure
 */
struct program_options parse_program_options(int argc, char *argv[])
{
	struct program_options program_options;
	program_options.mode = MODE_UNKNOWN;
	program_options.input_name = NULL;
	program_options.output_name = NULL;

	int i;
	for (i = 1; i < argc; i++) {
		char *argument = argv[i];

		// Parse options
		if (strcmp(argument, "--help") == 0) {
			program_options.mode = MODE_HELP;
			break;
		}

		if (program_options.mode == MODE_UNKNOWN) {
			// Parse modes
			if (strcmp(argument, "pack") == 0) {
				program_options.mode = MODE_PACK;
				continue;
			}
			if (strcmp(argument, "list") == 0) {
				program_options.mode = MODE_LIST;
				continue;
			}
			if (strcmp(argument, "unpack") == 0) {
				program_options.mode = MODE_UNPACK;
				continue;
			}
			if (strcmp(argument, "help") == 0) {
				program_options.mode = MODE_HELP;
				break;
			}
		} else {
			// Parse INPUT and OUTPUT
			if ((program_options.mode == MODE_PACK) ||
			    (program_options.mode == MODE_LIST) ||
			    (program_options.mode == MODE_UNPACK)) {
				if (program_options.input_name == NULL) {
					program_options.input_name = argument;
					continue;
				}
				if (program_options.mode != MODE_LIST) {
					program_options.output_name = argument;
					continue;
				}
			}
		}

		// Unexpected argument
		fprintf(stderr, "Unexpected argument %s\n", argument);
		program_options.mode = MODE_UNKNOWN;
		break;
	}

	if ((program_options.mode == MODE_PACK) ||
	    (program_options.mode == MODE_LIST) ||
	    (program_options.mode == MODE_UNPACK)) {
		if (program_options.input_name == NULL) {
			fprintf(stderr,
				"INPUT is required, but was not given\n");
			program_options.mode = MODE_UNKNOWN;
		}
	}
	if ((program_options.mode == MODE_PACK) ||
	    (program_options.mode == MODE_UNPACK)) {
		if (program_options.output_name == NULL) {
			fprintf(stderr,
				"OUTPUT is required, but was not given\n");
			program_options.mode = MODE_UNKNOWN;
		}
	}

	return program_options;
}

int main(int argc, char *argv[])
{
	struct program_options program_options =
	    parse_program_options(argc, argv);

	switch (program_options.mode) {
	case MODE_PACK: {
		// TODO!
		struct file_data *input_directory_data =
		    list_directory(program_options.input_name);

		archive_ptr_t current_position = sizeof(struct archive_header);
		assign_archive_positions(input_directory_data,
					 &current_position);
		assign_archive_content_positions(input_directory_data,
						 &current_position);

		struct file_wrapper *output_file = file_creat(
		    program_options.output_name, S_IRUSR | S_IWUSR | S_IRGRP);
		if (output_file == NULL) {
			perror("file_creat() failed");
			exit(-1);
		}

		write_full_archive(input_directory_data, output_file);

		if (file_close(output_file) < 0) {
			perror("file_close() failed");
			exit(-1);
		}

		free_directory_tree(input_directory_data);

		break;
	}
	case MODE_LIST: {
		struct file_wrapper *input_file =
		    file_open(program_options.input_name, O_RDONLY);
		if (input_file == NULL) {
			perror("file_open() failed");
			exit(-1);
		}

		struct file_data *input_archive_data =
		    read_full_archive(input_file);

		if (file_close(input_file) < 0) {
			perror("file_close() failed");
			exit(-1);
		}

		print_directory_tree(input_archive_data, 0);

		free_directory_tree(input_archive_data);

		break;
	}
	case MODE_UNPACK: {
		struct file_wrapper *input_file =
		    file_open(program_options.input_name, O_RDONLY);
		if (input_file == NULL) {
			perror("file_open() failed");
			exit(-1);
		}

		struct file_data *input_archive_data =
		    read_full_archive(input_file);

		read_archive_content(input_archive_data, input_file,
				      program_options.output_name);

		if (file_close(input_file) < 0) {
			perror("file_close() failed");
			exit(-1);
		}

		free_directory_tree(input_archive_data);
		// TODO
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

