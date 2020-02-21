#include "file_wrapper.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>

struct file_wrapper*
file_open(const char* pathname, int flags)
{
    const int fd = open(pathname, flags);
    if (fd < 0)
        return NULL;
    struct file_wrapper* const result = malloc(sizeof(struct file_wrapper));
    result->fd = fd;
    result->flags = flags;

    struct stat stat_result;
    if (fstat(result->fd, &stat_result) < 0) {
        free(result);
        return NULL;
    }
    result->size = stat_result.st_size;

    const off_t position = lseek(result->fd, 0, SEEK_SET);
    if (position < 0) {
        free(result);
        return NULL;
    }
    result->position = position;

    return result;
}

struct file_wrapper*
file_open_with_mode(const char* pathname, int flags, mode_t mode)
{
    const int fd = open(pathname, flags, mode);
    if (fd < 0)
        return NULL;
    struct file_wrapper* const result = malloc(sizeof(struct file_wrapper));
    result->fd = fd;
    result->flags = flags;

    struct stat stat_result;
    if (fstat(result->fd, &stat_result) < 0) {
        free(result);
        return NULL;
    }
    result->size = stat_result.st_size;

    if (result->flags & O_APPEND) {
        result->position = result->size;
    } else
        result->position = 0;

    return result;
}

struct file_wrapper*
file_creat(const char* pathname, mode_t mode)
{
    return file_open_with_mode(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int
file_close(struct file_wrapper* file)
{
    if (file == NULL)
        return 0;
    const int result = close(file->fd);
    if (result < 0)
        return -1;
    free(file);
    return 0;
}

int
file_write(struct file_wrapper* file, const void* buf, size_t size)
{
    if (file == NULL) {
        errno = EINVAL;
        return -1;
    }

    ssize_t result = 0;
    while ((size_t)result < size) {
        buf = (void*)(((char*)buf) + result);

        result = write(file->fd, buf, size);

        if (result < 0)
            return -1; // TODO: get file position data

        size -= result;
        file->position += result;
        if (file->position > file->size)
            file->size = file->position;
    }

    return 0;
}

int
file_read(struct file_wrapper* file, void* buf, size_t size)
{
    if (file == NULL) {
        errno = EINVAL;
        return -1;
    }

    ssize_t result = 0;
    while ((size_t)result < size) {
        buf = (void*)(((char*)buf) + result);

        result = read(file->fd, buf, size);

        if (result < 0)
            return -1; // TODO: get file position data

        size -= result;
        file->position += result;
    }

    return 0;
}

int
file_seek(struct file_wrapper* file, off_t position)
{
    if (file == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (position > file->size) {
        errno = ERANGE;
        return -1;
    }
    if (file->flags & O_APPEND)
        return 0;
    if (position == file->position)
        return 0;

    const off_t result = lseek(file->fd, position, SEEK_SET);
    if (result < 0)
        return -1;
    file->position = result;

    return 0;
}

int
file_fetch_position(struct file_wrapper* file)
{
    if (file == NULL) {
        errno = EINVAL;
        return -1;
    }
    const off_t result = lseek(file->fd, 0, SEEK_SET);
    if (result < 0)
        return -1;
    file->position = result;

    return 0;
}

int
file_cat(struct file_wrapper* input_file,
         struct file_wrapper* output_file,
         size_t size,
         size_t buffer_size)
{
    char* const buffer = malloc(buffer_size);
    unsigned int portion_size = buffer_size;

    while (size > 0) {
        if (size < buffer_size) {
            portion_size = size;
        }
        if (file_read(input_file, buffer, portion_size) < 0)
            return -1;
        if (file_write(output_file, buffer, portion_size) < 0)
            return -1;
        size -= portion_size;
    }

    free(buffer);
    return 0;
}

