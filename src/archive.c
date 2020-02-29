#include "archive.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void
assign_archive_positions(struct file_data* file_data,
                         archive_ptr_t* position_ptr,
                         const struct program_parameters* program_parameters)
{
    struct file_data* current_file_data;

    for (current_file_data = file_data; current_file_data != NULL;
         current_file_data = current_file_data->next) {
        current_file_data->archive_position = *position_ptr;
        *position_ptr += sizeof(struct archive_entry_data);
        if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
            *position_ptr += sizeof(struct archive_directory_data);
            if (current_file_data->first_child != NULL)
                assign_archive_positions(current_file_data->first_child,
                                         position_ptr,
                                         program_parameters);
        } else {
            *position_ptr += sizeof(struct archive_file_data);
        }
    }
}

void
assign_archive_content_positions(
  struct file_data* file_data,
  archive_ptr_t* position_ptr,
  const struct program_parameters* program_parameters)
{
    struct file_data* current_file_data;

    for (current_file_data = file_data; current_file_data != NULL;
         current_file_data = current_file_data->next) {
        if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
            if (current_file_data->first_child != NULL)
                assign_archive_content_positions(current_file_data->first_child,
                                                 position_ptr,
                                                 program_parameters);
        } else {
            current_file_data->archive_content_position = *position_ptr;
            *position_ptr += current_file_data->file_size;
        }
    }
}

void
write_archive_headers(struct file_data* file_data,
                      struct file_wrapper* output_file,
                      const struct program_parameters* program_parameters)
{
    struct file_data* current_file_data;
    for (current_file_data = file_data; current_file_data != NULL;
         current_file_data = current_file_data->next) {
        struct archive_entry_data entry_data;
        entry_data.mode = current_file_data->file_mode;

        if (current_file_data->next == NULL) {
            entry_data.is_last = 1;
            entry_data.next_ptr = 0;
        } else {
            entry_data.is_last = 0;
            entry_data.next_ptr = current_file_data->next->archive_position;
        }

        entry_data.st_atim = current_file_data->st_atim;
        entry_data.st_mtim = current_file_data->st_mtim;
        entry_data.st_ctim = current_file_data->st_ctim;

        memset(&(entry_data.name), 0, 256);
        strcpy(entry_data.name, current_file_data->file_name);

        if (file_write(output_file,
                       &entry_data,
                       sizeof(struct archive_entry_data)) < 0) {
            print_perror(program_parameters, "file_write() failed");
        }

        if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
            print_info(program_parameters,
                       "Adding directory %s..\n",
                       current_file_data->file_access_path);

            struct archive_directory_data archive_directory_data;

            if (current_file_data->first_child != NULL) {
                archive_directory_data.is_empty = 0;
                archive_directory_data.first_child_ptr =
                  current_file_data->first_child->archive_position;
            } else {
                archive_directory_data.is_empty = 1;
                archive_directory_data.first_child_ptr = 0;
            }
            if (file_write(output_file,
                           &archive_directory_data,
                           sizeof(struct archive_directory_data)) < 0) {
                print_perror(program_parameters, "file_write() failed");
            }

            if (current_file_data->first_child != NULL) {
                write_archive_headers(current_file_data->first_child,
                                      output_file,
                                      program_parameters);
            }
        } else {
            print_info(program_parameters,
                       "Adding file %s...\n",
                       current_file_data->file_access_path);

            struct archive_file_data archive_file_data;
            archive_file_data.content_ptr =
              current_file_data->archive_content_position;
            archive_file_data.content_size = current_file_data->file_size;

            if (file_write(output_file,
                           &archive_file_data,
                           sizeof(struct archive_file_data)) < 0) {
                print_perror(program_parameters, "file_write() failed");
            }
        }
    }
}

void
write_archive_content(struct file_data* file_data,
                      struct file_wrapper* output_file,
                      const struct program_parameters* program_parameters)
{
    struct file_data* current_file_data;
    for (current_file_data = file_data; current_file_data != NULL;
         current_file_data = current_file_data->next) {
        if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
            if (current_file_data->first_child != NULL)
                write_archive_content(current_file_data->first_child,
                                      output_file,
                                      program_parameters);
        } else if ((current_file_data->file_mode & S_IFMT) == S_IFREG) {
            struct file_wrapper* const current_file =
              file_open(current_file_data->file_access_path, O_RDONLY);
            if (current_file == NULL)
                print_perror(program_parameters, "file_open() failed");

            if (file_cat(current_file,
                         output_file,
                         current_file_data->file_size,
                         program_parameters->file_cat_buffer_size) < 0)
                print_perror(program_parameters, "file_cat() failed");

            if (file_close(current_file) < 0)
                print_perror(program_parameters, "file_close() failed");
        } else if ((current_file_data->file_mode & S_IFMT) == S_IFLNK) {
            if (file_write(output_file,
                           current_file_data->symlink_target,
                           current_file_data->file_size) < 0)
                print_perror(program_parameters, "file_write() failed");
        }
    }
}

void
write_full_archive(struct file_data* file_data,
                   struct file_wrapper* output_file,
                   const struct program_parameters* program_parameters)
{
    struct archive_header header;
    memcpy(header.header_sign, ARCHIVE_HEADER_SIGN, ARCHIVE_HEADER_SIGN_SIZE);

    header.root_directory_ptr = file_data->archive_position;

    if (file_write(output_file, &header, sizeof(struct archive_header)) < 0) {
        print_perror(program_parameters, "file_write() failed");
    }

    write_archive_headers(file_data, output_file, program_parameters);
    write_archive_content(file_data, output_file, program_parameters);
}

int
check_file_name(const char* name, size_t buffer_size)
{
    if (memchr(name, 0, buffer_size) == NULL)
        return -1;
    if (strchr(name, '/') != NULL)
        return -1;
    if (strcmp(name, "..") == 0)
        return -1;
    return 0;
}

int
check_file_mode(mode_t mode)
{
    const mode_t file_type = mode & S_IFMT;
    if ((file_type != S_IFREG) && (file_type != S_IFDIR) &&
        (file_type != S_IFLNK)) {
        return -1;
    }
    return 0;
}

struct file_data*
read_archive_headers(const char* parent_path,
                     struct file_wrapper* input_file,
                     archive_ptr_t position,
                     const struct program_parameters* program_parameters)
{
    struct file_data* first_file_data = NULL;
    struct file_data* current_file_data = NULL;

    archive_ptr_t current_position = position;

    while (1) {
        if (current_position >= (archive_ptr_t)input_file->size) {
            print_error(program_parameters,
                        "Error: header position %lu is exceeding file "
                        "size %ld\n",
                        current_position,
                        input_file->size);
        }

        // Security check to avoid circular file archive pointers
        if ((off_t)current_position < input_file->position) {
            print_error(program_parameters, "Error: invalid header position\n");
        }

        if (file_seek(input_file, (off_t)current_position) < 0) {
            print_perror(program_parameters, "file_seek() failed");
        }

        struct archive_entry_data entry_header;
        if (file_read(
              input_file, &entry_header, sizeof(struct archive_entry_data)) < 0)
            print_perror(program_parameters, "file_read() failed");

        if (check_file_name(entry_header.name, sizeof(entry_header.name)) < 0) {
            entry_header.name[sizeof(entry_header.name) - 1] = '\0';
            print_error(program_parameters,
                        "Error: invalid file name %s\n",
                        entry_header.name);
        }

        if (check_file_mode((mode_t)entry_header.mode) < 0)
            print_error(program_parameters,
                        "Error: invalid file mode %x\n",
                        entry_header.mode);

        struct file_data* const data = malloc(sizeof(struct file_data));
        if (data == NULL)
            print_perror(program_parameters, "malloc() failed");

        data->first_child = NULL;
        data->next = NULL;
        data->file_size = 0;
        data->file_name = str_create_copy(entry_header.name);
        if (data->file_name == NULL)
            print_perror(program_parameters, "str_create_copy() failed");
        if (parent_path != NULL) {
            data->file_access_path =
              str_create_concat3(parent_path, "/", data->file_name);
            if (data->file_access_path == NULL)
                print_perror(program_parameters, "str_create_concat3() failed");
        } else {
            data->file_access_path = str_create_copy(data->file_name);
            if (data->file_access_path == NULL)
                print_perror(program_parameters, "str_create_copy() failed");
        }
        data->file_mode = (mode_t)entry_header.mode;
        data->symlink_target = NULL;
        data->st_atim = entry_header.st_atim;
        data->st_mtim = entry_header.st_mtim;
        data->st_ctim = entry_header.st_ctim;
        data->archive_position = current_position;

        if ((data->file_mode & S_IFMT) == S_IFDIR) {
            struct archive_directory_data directory_header;
            if (file_read(input_file,
                          &directory_header,
                          sizeof(struct archive_directory_data)) < 0) {
                print_perror(program_parameters, "file_read() failed");
            }

            if (directory_header.is_empty == 0) {
                data->first_child =
                  read_archive_headers(data->file_access_path,
                                       input_file,
                                       directory_header.first_child_ptr,
                                       program_parameters);
            }
        } else if (((data->file_mode & S_IFMT) == S_IFREG) ||
                   (data->file_mode & S_IFMT) == S_IFLNK) {
            struct archive_file_data file_header;
            if (file_read(input_file,
                          &file_header,
                          sizeof(struct archive_file_data)) < 0) {
                print_perror(program_parameters, "file_read() failed");
            }

            data->file_size = file_header.content_size;
            data->archive_content_position = file_header.content_ptr;
        } else {
            // TODO
            print_error(program_parameters,
                        "Error: invalid archive entry type "
                        "(should be either directory, symlink or file)\n");
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

void
read_archive_content(struct file_data* file_data,
                     struct file_wrapper* input_file,
                     const char* output_directory_name,
                     const struct program_parameters* program_parameters)
{
    struct file_data* current_file_data;
    for (current_file_data = file_data; current_file_data != NULL;
         current_file_data = current_file_data->next) {
        const mode_t file_mode =
          current_file_data->file_mode &
          (S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX);
        int need_time_change =
          ((current_file_data->file_mode & S_IFMT) == S_IFDIR) ||
          ((current_file_data->file_mode & S_IFMT) == S_IFREG);
        char* const file_path = str_create_concat3(
          output_directory_name, "/", current_file_data->file_access_path);
        if (file_path == NULL)
            print_perror(program_parameters, "str_create_concat3() failed");

        if ((current_file_data->file_mode & S_IFMT) == S_IFDIR) {
            print_info(
              program_parameters, "Extracting directory to %s...\n", file_path);

            if (mkdir(file_path, file_mode) < 0) {
                if (errno == EEXIST) {
                    errno = 0; // TODO
                } else {
                    print_perror(program_parameters, "mkdir() failed");
                }
            }

            if (current_file_data->first_child != NULL)
                read_archive_content(current_file_data->first_child,
                                     input_file,
                                     output_directory_name,
                                     program_parameters);

        } else if ((current_file_data->file_mode & S_IFMT) == S_IFREG) {
            print_info(
              program_parameters, "Extracting file to %s...\n", file_path);

            if (current_file_data->archive_content_position >=
                (archive_ptr_t)input_file->size) {
                print_error(program_parameters,
                            "Error: file content "
                            "position %lu is "
                            "exceeding file "
                            "size %ld\n",
                            current_file_data->archive_content_position,
                            input_file->size);
            }
            if ((current_file_data->archive_content_position +
                 current_file_data->file_size) >
                (archive_ptr_t)input_file->size) {
                print_error(program_parameters,
                            "Error: file content end "
                            "position %lu is "
                            "exceeding file "
                            "size %ld\n",
                            current_file_data->archive_content_position +
                              current_file_data->file_size,
                            input_file->size);
            }

            struct file_wrapper* const current_file =
              file_creat(file_path, file_mode);
            if (current_file == NULL) {
                print_perror(program_parameters, "file_creat() failed");
            }

            if (file_seek(input_file,
                          (off_t)current_file_data->archive_content_position) <
                0) {
                print_perror(program_parameters, "file_seek() failed");
            }
            if (file_cat(input_file,
                         current_file,
                         current_file_data->file_size,
                         program_parameters->file_cat_buffer_size) < 0) {
                print_perror(program_parameters, "file_cat() failed");
            }

            if (file_close(current_file) < 0) {
                print_perror(program_parameters, "file_close() failed");
            }
        } else if ((current_file_data->file_mode & S_IFMT) == S_IFLNK) {
            print_info(
              program_parameters, "Extracting symlink to %s...\n", file_path);

            if (current_file_data->archive_content_position >=
                (archive_ptr_t)input_file->size) {
                print_error(program_parameters,
                            "Error: file content "
                            "position %lu is "
                            "exceeding file "
                            "size %ld\n",
                            current_file_data->archive_content_position,
                            input_file->size);
            }
            if ((current_file_data->archive_content_position +
                 current_file_data->file_size) >
                (archive_ptr_t)input_file->size) {
                print_error(program_parameters,
                            "Error: file content end "
                            "position %lu is "
                            "exceeding file "
                            "size %ld\n",
                            current_file_data->archive_content_position +
                              current_file_data->file_size,
                            input_file->size);
            }

            current_file_data->symlink_target =
              malloc(current_file_data->file_size);
            if (current_file_data->symlink_target == NULL)
                print_perror(program_parameters, "malloc() failed");

            if (file_seek(input_file,
                          (off_t)current_file_data->archive_content_position) <
                0)
                print_perror(program_parameters, "file_seek() failed");
            if (file_read(input_file,
                          current_file_data->symlink_target,
                          current_file_data->file_size) < 0)
                print_perror(program_parameters, "file_read() failed");

            if (current_file_data
                  ->symlink_target[current_file_data->file_size - 1] != 0) {
                current_file_data
                  ->symlink_target[current_file_data->file_size - 1] = 0;
                print_error(program_parameters,
                            "Error: symlink target %s is not NULL-terminated\n",
                            current_file_data->symlink_target);
            }

            if (symlink(current_file_data->symlink_target, file_path) < 0) {
                print_perror(program_parameters, "symlink() failed");
            }
        }

        if (need_time_change) {
            struct timespec file_times[2];
            file_times[0] = current_file_data->st_atim;
            file_times[1] = current_file_data->st_mtim;
            if (utimensat(AT_FDCWD, file_path, file_times, 0) < 0) {
                print_perror(program_parameters, "utimensat() failed");
            }
        }

        free(file_path);
    }
}

struct file_data*
read_full_archive(struct file_wrapper* input_file,
                  const struct program_parameters* program_parameters)
{
    struct archive_header header;

    if (file_read(input_file, &header, sizeof(struct archive_header)) < 0) {
        print_perror(program_parameters, "file_read() failed");
    }
    if (memcmp(header.header_sign,
               ARCHIVE_HEADER_SIGN,
               ARCHIVE_HEADER_SIGN_SIZE) != 0) {
        print_error(program_parameters, "Error: invalid archive header\n");
    }

    return read_archive_headers(
      NULL, input_file, header.root_directory_ptr, program_parameters);
}

