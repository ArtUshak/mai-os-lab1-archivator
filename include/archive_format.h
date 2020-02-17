#ifndef ARCHIVE_FORMAT_H_INCLUDED
#define ARCHIVE_FORMAT_H_INCLUDED

#include <sys/types.h>

#include <stdint.h>

typedef uint64_t archive_ptr_t;

/* Main archive header, should be present at beginning of file.
 */
struct archive_header {
	uint8_t header_sign[16]; // Magic signature data
	archive_ptr_t root_directory_ptr;
};

/* Header for archive entry (file or directory).
 */
struct archive_entry_data {
	uint32_t mode; // file mode (can be used to determine whether it is file
		       // or directory)
	uint8_t is_last; // 1 if file or directory is last in its parent
			 // directory (does not have next sibling), 0 otherwise
	archive_ptr_t next_ptr;	 // address of archive_entry_data for next
				 // sibling in archive file
	char name[256];		 // file name // TODO
	struct timespec st_atim; // access time
	struct timespec st_mtim; // modification time
	struct timespec st_ctim; // creation time
};

/* Header for archive file entry, should be present after archive_entry_data for
 * files.
 */
struct archive_file_data {
	archive_ptr_t
	    content_ptr; // address of file content beginning in archive file
	archive_ptr_t content_size; // size of file
};

/* Header for archive directory entry, should be present after
 * archive_entry_data for directories.
 */
struct archive_directory_data {
	uint8_t is_empty; // 1 if directory is empty (does not contain any files
			  // or subfolder), 0 otherwise
	archive_ptr_t first_child_ptr; // address of archive_entry_data for first child (file or subfolder)
};

#endif

