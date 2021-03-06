#ifndef FILE_WRAPPER_H_INCLUDED
#define FILE_WRAPPER_H_INCLUDED

#include <sys/types.h>
#include <unistd.h>

/* Custom wrapper for Linux files, alternative to FILE from C standard library.
 */
struct file_wrapper
{
    unsigned int fd; // file descriptor
    int flags;       // file open flags
    off_t size;      // file size in bytes
    off_t position;  // current position in file in bytes from beginning
};

/* Open file with flags, create file_wrapper structure for that file and return
 * pointer to it. If file can not be opened, return NULL.
 */
struct file_wrapper* file_open(const char* pathname, int flags);

/* Open file with flags and mode, create file_wrapper structure for that file
 * and return pointer to it. If file can not be opened, return NULL.
 */
struct file_wrapper* file_open_with_mode(const char* pathname,
                                         int flags,
                                         mode_t mode);

/* Create file with mode, create file_wrapper structure for that file and return
 * pointer to it. If file can not be created, return NULL.
 */
struct file_wrapper* file_creat(const char* pathname, mode_t mode);

/* Close file related to file_wrapper structure and deallocate this structure.
 */
int file_close(struct file_wrapper* file);

/* Write data from pointer buf, of size bytes to file related to file_wrapper
 * structure. Return 0 on success, -1 on error.
 */
int file_write(struct file_wrapper* file, const void* buf, size_t size);

/* Read data to pointer buf, of size bytes to file related to file_wrapper
 * structure. Return 0 on success, -1 on error.
 */
int file_read(struct file_wrapper* file, void* buf, size_t size);

/* Seek position in file related to file_wrapper. Return 0 on success, -1 on
 * error.
 */
int file_seek(struct file_wrapper* file, off_t position);

/* Update position in file_wrapper. Return 0 on success, -1 on error.
 */
int file_fetch_position(struct file_wrapper* file);

/* Write data of given size from input_file to output_file using buffer of size
 * buffer_size. Return 0 on success, -1 on error.
 */
int file_cat(struct file_wrapper* input_file,
             struct file_wrapper* output_file,
             size_t size,
             size_t buffer_size);

#endif

