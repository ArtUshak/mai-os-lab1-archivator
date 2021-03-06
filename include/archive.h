#ifndef ARCHIVE_H_INCLUDED
#define ARCHIVE_H_INCLUDED

#include <sys/types.h>

#include "archive_format.h"
#include "file_wrapper.h"
#include "listdir.h"
#include "program_options.h"

/* Assign archive_position field to file_data and its children and next entries
 * recursively. First available position address in archive is stored in value
 * referenced by position_ptr.
 */
void assign_archive_positions(
  struct file_data* file_data,
  archive_ptr_t* position_ptr,
  const struct program_parameters* program_parameters);

/* Assign archive_content_position field to files in file_data and following
 * entries recursively (to files only). First available position address in
 * archive is stored in value referenced by position_ptr.
 */
void assign_archive_content_positions(
  struct file_data* file_data,
  archive_ptr_t* position_ptr,
  const struct program_parameters* program_parameters);

/* Write archive entry, file and directory headers to output_file recursively.
 */
void write_archive_headers(struct file_data* file_data,
                           struct file_wrapper* output_file,
                           const struct program_parameters* program_parameters);

/* Write archive file contents to output_file recursively.
 */
void write_archive_content(struct file_data* file_data,
                           struct file_wrapper* output_file,
                           const struct program_parameters* program_parameters);

/* Write archive to to output_file.
 */
void write_full_archive(struct file_data* file_data,
                        struct file_wrapper* output_file,
                        const struct program_parameters* program_parameters);

/* Return 0 if name is correct ext4 filename and is not "..", -1 otherwise.
 */
int check_file_name(const char* name, size_t buffer_size);

/* Return 0 if mode is correct ext4 file mode allowed in archive, -1 otherwise.
 */
int check_file_mode(mode_t mode);

/* Read archive entry, file and directory headers from input_file recursively
 * starting from position.
 */
struct file_data* read_archive_headers(
  const char* parent_path,
  struct file_wrapper* input_file,
  archive_ptr_t position,
  const struct program_parameters* program_parameters);

/* Read archive main header and entry, file and directory headers and return
 * directory tree.
 */
struct file_data* read_full_archive(
  struct file_wrapper* input_file,
  const struct program_parameters* program_parameters);

/* Extract file content from archive recursively.
 */
void read_archive_content(struct file_data* file_data,
                          struct file_wrapper* input_file,
                          const char* output_directory_name,
                          const struct program_parameters* program_parameters);

#endif

