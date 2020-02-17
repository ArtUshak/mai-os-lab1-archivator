#ifndef FILEWRAPPER_H_INCLUDED
#define FILEWRAPPER_H_INCLUDED

#include <sys/types.h>
#include <unistd.h>

enum file_state { OPEN, CLOSED };

struct file_wrapper {
	unsigned int fd;
	int flags;
	off_t size;
	off_t position;
};

/* Open file with flags, create file_wrapper structure for that file and return
 * pointer to it. If file can not be opened, return NULL.
 */
struct file_wrapper *file_open(const char *pathname, int flags);

/* Open file with flags and mode, create file_wrapper structure for that file
 * and return pointer to it. If file can not be opened, return NULL.
 */
struct file_wrapper *file_open_with_mode(const char *pathname, int flags,
					 mode_t mode);

/* Create file with mode, create file_wrapper structure for that file and return
 * pointer to it. If file can not be created, return NULL.
 */
struct file_wrapper *file_creat(const char *pathname, mode_t mode);

/* Close file related to file_wrapper structure and deallocate this structure.
 */
int file_close(struct file_wrapper *file);

/* Write data from pointer buf, of size bytes to file related to file_wrapper
 * structure. Return 0 on success, -1 on error.
 */
int file_write(struct file_wrapper *file, const void *buf, size_t size);

/* Read data to pointer buf, of size bytes to file related to file_wrapper
 * structure. Return 0 on success
 */
int file_read(struct file_wrapper *file, void *buf, size_t size);

/* Seek position in file related to file_wrapper.
 */
int file_seek(struct file_wrapper *file, off_t position);

/* Write data of given size from input_file to output_file using buffer of size
 * buffer_size.
 */
int file_cat(struct file_wrapper *input_file, struct file_wrapper *output_file,
	     size_t size, size_t buffer_size);

#endif

