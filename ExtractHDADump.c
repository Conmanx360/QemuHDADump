#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	char *dir_str;
	uint32_t frame_cnt;

	uint64_t *rirb_buf;
	uint32_t *corb_buf;

	char *file_name;
	char *cur_file_str;
	uint32_t file_name_len;

	FILE *frame_file;
	uint32_t cur_offset;
	uint32_t cur_frame;
} hda_dump_data;

static uint32_t allocate_buffers(hda_dump_data *data)
{
	/*
	 * Allocate a buffer to store the frame file name string along with
	 * the directory string.
	 */
	data->file_name = calloc(strlen(data->dir_str) + data->file_name_len + 1, sizeof(char));
	if (!data->file_name) {
		printf("Failed to allocate memory for filename!\n");
		return 1;
	}

	data->corb_buf = calloc(data->frame_cnt * 0x100, sizeof(*data->corb_buf));
	if (!data->corb_buf) {
		printf("Failed to allocate corb buffer!\n");
		return 1;
	}

	data->rirb_buf = calloc(data->frame_cnt * 0x100, sizeof(*data->rirb_buf));
	if (!data->rirb_buf) {
		printf("Failed to allocate rirb buffer!\n");
		return 1;
	}

	return 0;
}

static void get_frame_cnt_from_dir(hda_dump_data *data)
{
	uint32_t highest_frame, tmp;
	struct dirent *tmp_dirent;
	char *file_name, *end_ptr;
	DIR *frame_dir;

	frame_dir = opendir(data->dir_str);
	if (!frame_dir) {
		printf("Directory open failed!\n");
		return;
	}

	highest_frame = 0;
	while ((tmp_dirent = readdir(frame_dir))) {
		file_name = tmp_dirent->d_name;
		if (strncmp("frame", file_name, 5))
			continue;

		tmp = strtol(file_name + 5, &end_ptr, 10);
		if (*end_ptr != '\0')
			continue;

		if (tmp > highest_frame) {
			data->file_name_len = strlen(file_name);
			highest_frame = tmp;
		}
	}

	closedir(frame_dir);

	data->frame_cnt = highest_frame + 1;
}

static uint32_t find_frame_file_str_from_dir(hda_dump_data *data)
{
	struct dirent *tmp_dirent;
	char *file_name, *end_ptr;
	DIR *frame_dir;
	uint32_t tmp;

	frame_dir = opendir(data->dir_str);
	if (!frame_dir) {
		printf("Directory open failed!\n");
		return 1;
	}

	while ((tmp_dirent = readdir(frame_dir))) {
		file_name = tmp_dirent->d_name;
		if (strncmp("frame", file_name, 5))
			continue;

		tmp = strtol(file_name + 5, &end_ptr, 10);
		if (*end_ptr != '\0')
			continue;

		if (tmp == data->cur_frame) {
			data->cur_file_str = strdup(file_name);
			break;
		}
	}

	closedir(frame_dir);

	if (!data->cur_file_str)
		return 1;

	return 0;
}

static uint32_t write_file_to_buffer(hda_dump_data *data)
{
	uint32_t words_read;
	FILE *tmp;

	if (find_frame_file_str_from_dir(data)) {
		printf("Failed to find frame %d, aborting.\n", data->cur_frame);
		return 1;
	}

	sprintf(data->file_name, "%s/%s", data->dir_str, data->cur_file_str);
	tmp = fopen(data->file_name, "r+");

	free(data->cur_file_str);

	if (!tmp) {
		printf("Failed to open frame file!\n");
		return 1;
	}

	words_read = fread(data->corb_buf + (data->cur_frame * 0x100),
			sizeof(*data->corb_buf), 0x100, tmp);
	if (words_read != 0x100) {
		printf("Incorrect amount of data read!\n");
		fclose(tmp);
		return 1;
	}

	/* Reset the file to the beginning, then get the RIRB offset. */
	rewind(tmp);
	fseek(tmp, 0x800, 0);

	words_read = fread(data->rirb_buf + (data->cur_frame * 0x100),
			sizeof(*data->rirb_buf), 0x100, tmp);
	if (words_read != 0x100) {
		printf("Incorrect amount of data read!\n");
		fclose(tmp);
		return 1;
	}

	fclose(tmp);

	return 0;
}

int main(int argc, char **argv)
{
	hda_dump_data data;
	int ret = 0;
	uint32_t i;

	/* Open the directory which the frame dumps are located in.*/
	if (argc < 2) {
		printf("Usage: %s folder_containing_framedumps\n", argv[0]);
		return -1;
	}

	memset(&data, 0, sizeof(data));

	data.dir_str = argv[1];
	get_frame_cnt_from_dir(&data);

	if (!data.frame_cnt) {
		printf("Failed to find frame files in directory!\n");
		ret = -1;
		goto exit;
	}

	if (allocate_buffers(&data)) {
		ret = -1;
		goto exit;
	}

	for (i = 0; i < data.frame_cnt; i++) {
		if (write_file_to_buffer(&data)) {
			ret = -1;
			goto exit;
		}

		data.cur_frame++;
	}

exit:
	if (data.file_name)
		free(data.file_name);

	if (data.corb_buf)
		free(data.corb_buf);

	if (data.rirb_buf)
		free(data.rirb_buf);

	return ret;
}
