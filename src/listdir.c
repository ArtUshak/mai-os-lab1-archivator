#include "listdir.h"

#include <fcntl.h>
#include <fts.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "program_options.h"
#include "util.h"

struct file_data*
list_directory_by_fts(FTS* ftsp,
                      const struct program_parameters* program_parameters)
{
    struct file_data* first_file_data = NULL;
    struct file_data* current_file_data = NULL;

    while (1) {
        FTSENT* const ftsent = fts_read(ftsp);

        if (ftsent == NULL) {
            if (errno != 0)
                print_perror(program_parameters, "fts_read() failed");
            break;
        }

        if (ftsent->fts_errno != 0) {
            errno = ftsent->fts_errno;
            print_perror(program_parameters, "fts_read() failed");
        }

        if (ftsent->fts_info == FTS_DP) {
            break;
        }

        if (check_file_mode(ftsent->fts_statp->st_mode) < 0)
            continue;
        if ((program_parameters->symlink_mode == SYMLINK_MODE_IGNORE) &&
            ((ftsent->fts_statp->st_mode & S_IFMT) == S_IFLNK))
            continue;

        struct file_data* const data = malloc(sizeof(struct file_data));
        data->archive_position = 0;
        data->archive_content_position = 0;
        data->first_child = NULL;
        data->next = NULL;
        data->symlink_target = NULL;
        data->st_atim = ftsent->fts_statp->st_atim;
        data->st_mtim = ftsent->fts_statp->st_mtim;
        data->st_ctim = ftsent->fts_statp->st_ctim;
        data->file_name = str_create_copy(ftsent->fts_name);
        data->file_mode = ftsent->fts_statp->st_mode;
        data->file_size = ftsent->fts_statp->st_size;
        data->file_access_path = str_create_copy(ftsent->fts_path);

        if (ftsent->fts_info == FTS_D) {
            data->first_child = list_directory_by_fts(ftsp, program_parameters);
        } else if (ftsent->fts_info == FTS_SL) {
            char* symlink_target =
              do_readlinkat(AT_FDCWD, data->file_access_path);
            if (symlink_target == NULL)
                print_perror(program_parameters, "do_readlinkat() failed");
            data->symlink_target = symlink_target;
            data->file_size = strlen(symlink_target) + 1;
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
    }

    return first_file_data;
}

int
fts_compare_function(const FTSENT** file1, const FTSENT** file2)
{
    if ((file1 == NULL) && (file2 == NULL))
        return 0;
    if (file1 == NULL)
        return 1;
    if (file2 == NULL)
        return -1;
    if ((*file1)->fts_number == ((*file2)->fts_number))
        return 0;
    return ((*file1)->fts_number > ((*file2)->fts_number)) ? 1 : -1;
}

struct file_data*
list_directory(char* const* root_paths,
               const struct program_parameters* program_parameters)
{
    FTS* const fts = fts_open(root_paths,
                              FTS_PHYSICAL,
                              fts_compare_function); // TODO

    if (fts == NULL) {
        print_perror(program_parameters, "fts_open() failed");
    }

    struct file_data* const result =
      list_directory_by_fts(fts, program_parameters);

    if (fts_close(fts) < 0) {
        print_perror(program_parameters, "fts_close() failed");
    }

    return result;
}

void
free_directory_tree(struct file_data* data)
{
    if (data == NULL)
        return;

    if (data->file_name != NULL)
        free(data->file_name);

    if (data->file_access_path != NULL)
        free(data->file_access_path);

    if (data->symlink_target != NULL)
        free(data->symlink_target);

    if (data->next != NULL)
        free_directory_tree(data->next);

    if (data->first_child != NULL)
        free_directory_tree(data->first_child);

    free(data);
}

void
print_directory_tree(struct file_data* data, unsigned int identation)
{
    // TODO
    struct file_data* current_file_data;
    for (current_file_data = data; current_file_data != NULL;
         current_file_data = current_file_data->next) {
        unsigned int i;
        for (i = 0; i < identation; i++)
            putchar(' ');
        printf("%s mode=%02x size=%012ld name=%s path=%s\n",
               ((current_file_data->file_mode & S_IFMT) == S_IFDIR)
                 ? "Directory"
                 : "File",
               current_file_data->file_mode,
               current_file_data->file_size,
               current_file_data->file_name,
               current_file_data->file_access_path);

        if (current_file_data->first_child != NULL) {
            print_directory_tree(current_file_data->first_child,
                                 identation + 2);
        }
    }
}

