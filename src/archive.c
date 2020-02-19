#include "archive.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void assign_archive_positions(struct file_data *file_data,
			      archive_ptr_t *position_ptr)
{
	struct file_data *current_file_data;

	for (current_file_data = file_data; current_file_data != NULL;
	     current_file_data = current_file_data->next) {
		current_file_data->archive_position = *position_ptr;
		*position_ptr += sizeof(struct archive_entry_data);
		if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
			*position_ptr += sizeof(struct archive_directory_data);
			if (current_file_data->first_child != NULL)
				assign_archive_positions(
				    current_file_data->first_child,
				    position_ptr);
		} else {
			*position_ptr += sizeof(struct archive_file_data);
		}
	}
}

void assign_archive_content_positions(struct file_data *file_data,
				      archive_ptr_t *position_ptr)
{
	struct file_data *current_file_data;

	for (current_file_data = file_data; current_file_data != NULL;
	     current_file_data = current_file_data->next) {
		if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
			if (current_file_data->first_child != NULL)
				assign_archive_content_positions(
				    current_file_data->first_child,
				    position_ptr);
		} else {
			current_file_data->archive_content_position =
			    *position_ptr;
			*position_ptr += current_file_data->file_size;
		}
	}
}

void write_archive_headers(struct file_data *file_data,
			   struct file_wrapper *output_file)
{
	struct file_data *current_file_data;
	for (current_file_data = file_data; current_file_data != NULL;
	     current_file_data = current_file_data->next) {
		struct archive_entry_data entry_data;
		entry_data.mode = current_file_data->file_mode;

		if (current_file_data->next == NULL) {
			entry_data.is_last = 1;
			entry_data.next_ptr = 0;
		} else {
			entry_data.is_last = 0;
			entry_data.next_ptr =
			    current_file_data->next->archive_position;
		}

		entry_data.st_atim = current_file_data->st_atim;
		entry_data.st_mtim = current_file_data->st_mtim;
		entry_data.st_ctim = current_file_data->st_ctim;

		memset(&(entry_data.name), 0, 256);
		strcpy(entry_data.name, current_file_data->file_name);

		if (file_write(output_file, &entry_data,
			       sizeof(struct archive_entry_data)) < 0) {
			perror("file_write() failed");
			exit(-1);
		}

		if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
			struct archive_directory_data archive_directory_data;

			if (current_file_data->first_child != NULL) {
				archive_directory_data.is_empty = 0;
				archive_directory_data.first_child_ptr =
				    current_file_data->first_child
					->archive_position;
			} else {
				archive_directory_data.is_empty = 1;
				archive_directory_data.first_child_ptr = 0;
			}
			if (file_write(output_file, &archive_directory_data,
				       sizeof(struct archive_directory_data)) <
			    0) {
				perror("file_write() failed");
				exit(-1);
			}

			if (current_file_data->first_child != NULL) {
				write_archive_headers(
				    current_file_data->first_child,
				    output_file);
			}
		} else {
			struct archive_file_data archive_file_data;
			archive_file_data.content_ptr =
			    current_file_data->archive_content_position;
			archive_file_data.content_size =
			    current_file_data->file_size;

			if (file_write(output_file, &archive_file_data,
				       sizeof(struct archive_file_data)) < 0) {
				perror("file_write() failed");
				exit(-1);
			}
		}
	}
}

void write_archive_content(struct file_data *file_data,
			   struct file_wrapper *output_file)
{
	struct file_data *current_file_data;
	for (current_file_data = file_data; current_file_data != NULL;
	     current_file_data = current_file_data->next) {
		if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
			if (current_file_data->first_child != NULL) {
				write_archive_content(
				    current_file_data->first_child,
				    output_file);
			}
		} else {
			struct file_wrapper *current_file = file_open(
			    current_file_data->file_access_path, O_RDONLY);
			if (current_file == NULL) {
				perror("file_open() failed");
				exit(-1);
			}

			if (file_cat(current_file, output_file,
				     current_file_data->file_size,
				     FILE_CAT_BUFFER_SIZE) < 0) {
				perror("file_cat() failed");
				exit(-1);
			}

			if (file_close(current_file) < 0) {
				perror("file_close() failed");
				exit(-1);
			}
		}
	}
}

void write_full_archive(struct file_data *file_data,
			struct file_wrapper *output_file)
{
	struct archive_header header;
	memcpy(header.header_sign, ARCHIVE_HEADER_SIGN,
	       ARCHIVE_HEADER_SIGN_SIZE);

	header.root_directory_ptr = file_data->archive_position;

	if (file_write(output_file, &header, sizeof(struct archive_header)) <
	    0) {
		perror("file_write() failed");
		exit(-1);
	}

	write_archive_headers(file_data, output_file);
	write_archive_content(file_data, output_file);
}

int check_file_name(const char *name, size_t buffer_size)
{
	if (memchr(name, 0, buffer_size) == NULL)
		return -1;
	if (strchr(name, '/') != NULL)
		return -1;
	if (strcmp(name, "..") == 0)
		return -1;
	return 0;
}

int check_file_mode(mode_t mode)
{
	mode_t file_type = mode & S_IFMT;
	if ((file_type != S_IFREG) && (file_type != S_IFDIR)) {
		return -1;
	}
	return 0;
}

struct file_data *read_archive_headers(const char *parent_path,
				       struct file_wrapper *input_file,
				       archive_ptr_t position)
{
	struct file_data *first_file_data = NULL;
	struct file_data *current_file_data = NULL;

	archive_ptr_t current_position = position;

	while (1) {
		if (current_position >= (archive_ptr_t)input_file->size) {
			fprintf(stderr,
				"Error: header position %lu is exceeding file "
				"size %ld\n",
				current_position, input_file->size);
			exit(-1);
		}

		// Security check to avoid circular file archive pointers
		if ((off_t)current_position < input_file->position) {
			fprintf(stderr, "Error: invalid header position\n");
			exit(-1);
		}

		if (file_seek(input_file, (off_t)current_position) < 0) {
			perror("file_seek() failed");
			exit(-1);
		}

		struct archive_entry_data entry_header;
		if (file_read(input_file, &entry_header,
			      sizeof(struct archive_entry_data)) < 0) {
			perror("file_read() failed");
			exit(-1);
		}

		if (check_file_name(entry_header.name,
				    sizeof(entry_header.name)) < 0) {
			entry_header.name[sizeof(entry_header.name) - 1] = '\0';
			fprintf(stderr, "Error: invalid file name %s\n",
				entry_header.name);
			exit(-1);
		}

		if (check_file_mode((mode_t)entry_header.mode) < 0) {
			fprintf(stderr, "Error: invalid file mode %x\n",
				entry_header.mode);
			exit(-1);
		}

		struct file_data *data = malloc(sizeof(struct file_data));

		data->first_child = NULL;
		data->next = NULL;
		data->file_size = 0;
		data->file_name = str_create_copy(entry_header.name);
		if (parent_path != NULL)
			data->file_access_path = str_create_concat3(
			    parent_path, "/", data->file_name);
		else
			data->file_access_path =
			    str_create_copy(data->file_name);
		data->file_mode = (mode_t)entry_header.mode;
		data->st_atim = entry_header.st_atim;
		data->st_mtim = entry_header.st_mtim;
		data->st_ctim = entry_header.st_ctim;
		data->archive_position = current_position;

		if ((data->file_mode & S_IFMT) == S_IFDIR) {
			struct archive_directory_data directory_header;
			if (file_read(input_file, &directory_header,
				      sizeof(struct archive_directory_data)) <
			    0) {
				perror("file_read() failed");
				exit(-1);
			}

			if (directory_header.is_empty == 0) {
				data->first_child = read_archive_headers(
				    data->file_access_path, input_file,
				    directory_header.first_child_ptr);
			}
		} else if ((data->file_mode & S_IFMT) == S_IFREG) {
			struct archive_file_data file_header;
			if (file_read(input_file, &file_header,
				      sizeof(struct archive_file_data)) < 0) {
				perror("file_read() failed");
				exit(-1);
			}

			data->file_size = file_header.content_size;
			data->archive_content_position =
			    file_header.content_ptr;
		} else {
			// TODO
			fprintf(stderr,
				"Error: invalid archive entry type "
				"(should be either directory or file)\n");
			exit(-1);
		}

		if (first_file_data == NULL) {
			first_file_data = data;
			current_file_data = data;
		} else {
			if (current_file_data != NULL) {
				current_file_data->next = data;
			}
			current_file_data = data;
		}

		if (entry_header.is_last == 0) {
			current_position = entry_header.next_ptr;
		} else
			break;
	}

	return first_file_data;
}

void read_archive_content(struct file_data *file_data,
			  struct file_wrapper *input_file,
			  const char *output_directory_name)
{
	struct file_data *current_file_data;
	for (current_file_data = file_data; current_file_data != NULL;
	     current_file_data = current_file_data->next) {
		mode_t file_mode =
		    current_file_data->file_mode &
		    (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX);
		char *file_path =
		    str_create_concat3(output_directory_name, "/",
				       current_file_data->file_access_path);

		if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
			fprintf(stderr, "Extracting directory to %s...\n",
				file_path);

			if (mkdir(file_path, file_mode) < 0) {
				if (errno == EEXIST) {
					errno = 0; // TODO
				} else {
					perror("mkdir() failed");
					exit(-1);
				}
			}

			if (current_file_data->first_child != NULL)
				read_archive_content(
				    current_file_data->first_child, input_file,
				    output_directory_name);

		} else {
			fprintf(stderr, "Extracting file to %s...\n",
				file_path);

			if (current_file_data->archive_content_position >=
			    (archive_ptr_t)input_file->size) {
				fprintf(
				    stderr,
				    "Error: file content "
				    "position %lu is "
				    "exceeding file "
				    "size %ld\n",
				    current_file_data->archive_content_position,
				    input_file->size);
				exit(-1);
			}
			if ((current_file_data->archive_content_position +
			     current_file_data->file_size) >
			    (archive_ptr_t)input_file->size) {
				fprintf(stderr,
					"Error: file content end "
					"position %lu is "
					"exceeding file "
					"size %ld\n",
					current_file_data
						->archive_content_position +
					    current_file_data->file_size,
					input_file->size);
				exit(-1);
			}

			struct file_wrapper *current_file =
			    file_creat(file_path, file_mode);
			if (current_file == NULL) {
				perror("file_creat() failed");
				exit(-1);
			}

			if (file_seek(input_file,
				      (off_t)current_file_data
					  ->archive_content_position) < 0) {
				perror("file_seek() failed");
				exit(-1);
			}
			if (file_cat(input_file, current_file,
				     current_file_data->file_size,
				     FILE_CAT_BUFFER_SIZE) < 0) {
				perror("file_cat() failed");
				exit(-1);
			}

			if (file_close(current_file) < 0) {
				perror("file_close() failed");
				exit(-1);
			}
		}

		struct timespec file_times[2];
		file_times[0] = current_file_data->st_atim;
		file_times[1] = current_file_data->st_mtim;
		if (utimensat(AT_FDCWD, file_path, file_times, 0) < 0) {
			perror("utimensat() failed");
			exit(-1);
		}

		free(file_path);
	}
}

struct file_data *read_full_archive(struct file_wrapper *input_file)
{
	struct archive_header header;

	if (file_read(input_file, &header, sizeof(struct archive_header)) < 0) {
		perror("file_read() failed");
		exit(-1);
	}
	if (memcmp(header.header_sign, ARCHIVE_HEADER_SIGN,
		   ARCHIVE_HEADER_SIGN_SIZE) != 0) {
		fprintf(stderr, "Error: invalid archive header\n");
		exit(-1);
	}

	return read_archive_headers(NULL, input_file,
				    header.root_directory_ptr);
}

