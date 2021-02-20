#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>


#define DEFAULT_BUF_SIZE 0x100
#define CORB_BUFF_SIZE 12
#define REGION_ZERO 0
#define BAR_REGION_OFFSET 40
#define BAR_ADDRESS_OFFSET 44
#define ARRAY_SIZE(array) \
    (sizeof(array) / sizeof(*array))

typedef struct {
	int fd;

	char *buf;
	size_t buf_size;

	uint32_t line_offset;

	uint64_t corb_buf_addr;
	uint32_t bar_region;
	uint32_t bar_addr;
	uint32_t bar_data;
	uint32_t bar_bytes;

	uint32_t corb_cnt;
	uint32_t frame_cnt;
} hda_dump_data;

static const char time_chk_chars[4] = { '@', '.', ':', '(' };

static void get_trace_line_offset(hda_dump_data *data)
{
	uint32_t i, match_cnt;

	for (i = match_cnt = 0; i < strlen(data->buf); i++) {
		if (data->buf[i] == time_chk_chars[match_cnt])
			match_cnt++;

		if (match_cnt == ARRAY_SIZE(time_chk_chars)) {
			data->line_offset = i;
			break;
		}
	}

	/* Now, find the end of the PCI bus ID and add that to the offset. */
	if (data->line_offset) {
		match_cnt = 0;
		for (i = data->line_offset; i < strlen(data->buf); i++) {
			if (data->buf[i] == ':')
				match_cnt++;

			if (match_cnt == 3) {
				data->line_offset = ++i;
				break;
			}
		}
	}
}

static void extract_data_from_qemu_trace_line(hda_dump_data *data)
{
	char *cur_char, *val_end;

	if (!data->line_offset)
		get_trace_line_offset(data);

	if (!data->line_offset)
		return;

	data->bar_region = data->bar_addr = data->bar_data = data->bar_bytes = 0;
	cur_char = data->buf + data->line_offset + 6;
	data->bar_region = (*cur_char) - '0';
	cur_char++;

	/* Now get bar address. */
	data->bar_addr = strtol(cur_char, &val_end, 16);
	cur_char = val_end + 1;
	if (*val_end != ',')
		return;

	/* Now get data value. */
	data->bar_data = strtol(cur_char, &val_end, 16);
	cur_char = val_end + 1;
	if (*val_end != ',')
		return;

	/* Now, get the byte length of the write. */
	data->bar_bytes = strtol(cur_char, NULL, 10);
}

void dump_current_frame(hda_dump_data *data, int is_final)
{
	const char *nl = "\n";
	char buf[0x100];
	uint32_t i;

	if (!data->corb_buf_addr) {
		printf("Buffer address not yet set, not dumping.\n");
		return;
	}

	memset(buf, 0, sizeof(buf));

	if (is_final)
		sprintf(buf, "pmemsave 0x%lx 0x1000 exit_frame\n", data->corb_buf_addr);
	else
		sprintf(buf, "pmemsave 0x%lx 0x1000 frame%04d\n", data->corb_buf_addr, data->frame_cnt);

	/*
	 * The ioctl expects a const char *, and writes out character by
	 * character, not by string, so that's how we'll have to write it out.
	 */
	for (i = 0; i < strlen(buf); i++)
		ioctl(data->fd, TIOCSTI, (const char *)&buf[i]);

	/* Write out a final newline character. */
	ioctl(data->fd, TIOCSTI, nl);
}

static void handle_hda_region_write(hda_dump_data *data)
{
	uint64_t tmp;

	switch (data->bar_addr) {
	case 0x20:
		if ((data->bar_data & 0x40000000) && (data->corb_cnt > 20))
			dump_current_frame(data, 1);

		break;

	case 0x40:
		tmp = data->bar_data;
		data->corb_buf_addr |= data->bar_data;

		printf("CORB buffer addr 0x%016lx.\n", data->corb_buf_addr);
		break;

	case 0x44:
		tmp = data->bar_data;
		tmp <<= 32;
		data->corb_buf_addr |= tmp;

		printf("CORB buffer addr 0x%016lx.\n", data->corb_buf_addr);
		break;

	case 0x48:
		if (data->bar_data == 0xff) {
			dump_current_frame(data, 0);
			printf("Call memory dump, frame_cnt %d.\n", data->frame_cnt++);
		}

		if (!data->corb_cnt)
			data->corb_cnt = data->bar_data & 0xff;
		else
			data->corb_cnt++;

		printf("CORB dump addr 0x%08x.\n", data->corb_cnt * 4);

		break;
	default:
		printf("bar_region %d, addr 0x%03x, data 0x%08x, bytes %d.\n", data->bar_region, data->bar_addr,
				data->bar_data, data->bar_bytes);

		break;
	}
}

int main(int argc, char *argv[])
{
	hda_dump_data data;
	char *tty_str;

	memset(&data, 0, sizeof(data));

	if (argc < 2)
		tty_str = "/dev/tty";
	else
		tty_str = argv[1];

	data.fd = open(tty_str, O_RDWR);

        if (data.fd < 0)
		return 1;

	data.buf_size = DEFAULT_BUF_SIZE;
	data.buf = calloc(data.buf_size, sizeof(*data.buf));

	while (1) {
		memset(data.buf, 0, data.buf_size);
		if (getline(&data.buf, &data.buf_size, stdin) == -1)
			break;

		extract_data_from_qemu_trace_line(&data);

		switch (data.bar_region) {
		case 0:
			handle_hda_region_write(&data);
			break;

		default:
			printf("bar_region %d, addr 0x%03x, data 0x%08x, bytes %d.\n",
					data.bar_region, data.bar_addr, data.bar_data,
					data.bar_bytes);
			break;
		}

		fflush(stdout);
	}

	if (data.buf)
		free(data.buf);

	if (data.fd)
		close(data.fd);

	return 0;
}
