#include "listdir.h"

#include <fts.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

struct file_data *list_directory_by_fts(FTS *ftsp)
{
	struct file_data *first_file_data = NULL;
	struct file_data *current_file_data = NULL;

	while (1) {
		FTSENT *ftsent = fts_read(ftsp);

		if (ftsent == NULL) {
			if (errno != 0) {
				perror("fts_read() failed");
				exit(-1);
			}
			break;
		}

		if (ftsent->fts_errno != 0) {
			errno = ftsent->fts_errno;
			perror("fts_read() failed");
			exit(-1);
		}

		if (ftsent->fts_info == FTS_DP) {
			break;
		}

		if (((ftsent->fts_statp->st_mode & S_IFMT) != S_IFREG) &
		    ((ftsent->fts_statp->st_mode & S_IFMT) != S_IFDIR))
			continue;

		struct file_data *data = malloc(sizeof(struct file_data));
		data->archive_position = 0;
		data->archive_content_position = 0;
		data->first_child = NULL;
		data->next = NULL;
		data->st_atim = ftsent->fts_statp->st_atim;
		data->st_mtim = ftsent->fts_statp->st_mtim;
		data->st_ctim = ftsent->fts_statp->st_ctim;
		data->file_name = str_create_copy(ftsent->fts_name);
		data->file_mode = ftsent->fts_statp->st_mode;
		data->file_size = ftsent->fts_statp->st_size;
		data->file_access_path = str_create_copy(ftsent->fts_path);

		if (ftsent->fts_info == FTS_D) {
			data->first_child = list_directory_by_fts(ftsp);
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

int fts_compare_function(const FTSENT **file1, const FTSENT **file2)
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

struct file_data *list_directory(char *directory_name)
{
	char *directory_path_array[2];
	directory_path_array[0] = directory_name;
	directory_path_array[1] = NULL;

	FTS *fts = fts_open(directory_path_array, FTS_PHYSICAL,
			    fts_compare_function); // TODO

	if (fts == NULL) {
		perror("fts() failed");
		exit(-1);
	}

	struct file_data *result = list_directory_by_fts(fts);

	return result;
}

void free_directory_tree(struct file_data *data)
{
	if (data == NULL)
		return;

	if (data->file_name != NULL)
		free(data->file_name);

	if (data->file_access_path != NULL)
		free(data->file_access_path);

	if (data->next != NULL)
		free_directory_tree(data->next);

	if (data->first_child != NULL)
		free_directory_tree(data->first_child);

	free(data);
}

void print_directory_tree(struct file_data *data, unsigned int identation)
{
	struct file_data *current_file_data;
	for (current_file_data = data; current_file_data != NULL;
	     current_file_data = current_file_data->next) {
		unsigned int i;
		for (i = 0; i < identation; i++)
			putchar(' ');
		printf("ELEMENT MODE=%02x SIZE=%012ld NAME=%s PATH=%s\n",
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

