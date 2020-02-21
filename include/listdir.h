#ifndef LISTDIR_H_INCLUDED
#define LISTDIR_H_INCLUDED

#include <sys/types.h>

#include <fts.h>

#include "archive_format.h"
#include "program_options.h"

/* File or directory data.
 */
struct file_data
{
    char* file_access_path; // full path related to current directory
    char* file_name;        // name of file or directory
    mode_t file_mode;       // file mode (can be used to determine whether it is
                            // file or directory)
    off_t file_size; // size (for files only, otherwise it should be set to
                     // NULL)
    struct timespec st_atim; // access time
    struct timespec st_mtim; // modification time
    struct timespec st_ctim; // status change time

    struct file_data* next; // pointer to next sibling file or directory data
    struct file_data*
      first_child; // pointer to data of first file or subdirectory in
    // this directory (for non-empty directories only,
    // otherwise it should be set to NULL)

    archive_ptr_t archive_position; // position of corresponding header in
                                    // archive file
    archive_ptr_t archive_content_position; // position of file content data in
                                            // archive file (for files only)
};

/* Populate directory tree recursively using FTS.
 */
struct file_data* list_directory_by_fts(
  FTS* ftsp,
  const struct program_parameters* program_parameters);

/* Build directory tree with given root pathes recursively and return it. Only
 * files and directories will be added to directory tree, symlinks, block
 * devices and others will be ignored.
 */
struct file_data* list_directory(char* const* root_paths,
                                 const struct program_parameters* program_parameters);

/* Deallocate memory used for directory tree.
 */
void free_directory_tree(struct file_data* ptr);

/* Print directory tree recursively.
 */
void print_directory_tree(struct file_data* ptr, unsigned int identation);

#endif

