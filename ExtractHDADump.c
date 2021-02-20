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

int main(int argc, char **argv)
{
	hda_dump_data data;
	int ret = 0;

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

exit:
	if (data.file_name)
		free(data.file_name);

	if (data.corb_buf)
		free(data.corb_buf);

	if (data.rirb_buf)
		free(data.rirb_buf);

	return ret;
}
