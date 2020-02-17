#ifndef LISTDIR_H_INCLUDED
#define LISTDIR_H_INCLUDED

#include <sys/types.h>

#include "archive_format.h"

/* File or directory data.
 */
struct file_data {
	char *file_access_path; // full path related to current directory
	char *file_name;	// name of file or directory
	mode_t file_mode; // file mode (can be used to determine whether it is
			  // file or directory)
	off_t file_size;  // size (for files only, otherwise it should be set to
			  // NULL)
	struct timespec st_atim; // access time
	struct timespec st_mtim; // modification time
	struct timespec st_ctim; // creation time

	struct file_data
	    *next; // pointer to next sibling file or directory data
	struct file_data
	    *first_child; // pointer to data of first file or subdirectory in
			  // this directory (for non-empty directories only,
			  // otherwise it should be set to NULL)

	archive_ptr_t archive_position; // position of corresponding header in
					// archive file
	archive_ptr_t
	    archive_content_position; // position of file content data in
				      // archive file (for files only)
};

/* Build directory tree with directory and all its files and subdirectories
 * recursively and return it. Only files and directories will be added to
 * directory tree, symlinks, block devices and others will be ignored.
 */
struct file_data *list_directory(char *directory_name);

/* Deallocate memory used for directory tree.
 */
void free_directory_tree(struct file_data *ptr);

/* Print directory tree recursively.
 */
void print_directory_tree(struct file_data *ptr, unsigned int identation);

#endif

