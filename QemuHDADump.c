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

void dumpMem(char *array, unsigned short framenumber, int fd, int is_final)
{
	int i;
	// int file = fd;
	unsigned short frameno = framenumber;
	unsigned int digit_one, digit_two;

	char *nl = "\n";
	// char stop[] = "stop\n";
	// char cont[] = "cont\n";
	char *pmemsave_part1 = "pmemsave ";
	char *pmemsave_part2 = " 0x1000 ";
	char *frame = "frame";
	char *final = "exit_dump";
	char frameChar[] = "00\0";

        if (array[0] == '\0')
	{
	  printf("dumpMem entered... but the address is not set, skipping\n");
	  return;
        } else
	  printf("dumpMem entered...\n");

	for(i = 0; pmemsave_part1[i]; i++) {
		ioctl(fd, TIOCSTI, pmemsave_part1+i);
	}

	for(i = 0; i < 18 && array[i]; i++) {
		ioctl(fd, TIOCSTI, array+i);
	}

	for(i = 0; pmemsave_part2[i]; i++) {
		ioctl(fd, TIOCSTI, pmemsave_part2+i);
	}

	if(is_final) {
		for(i = 0; final[i]; i++) {
			ioctl(fd, TIOCSTI, final+i);
		}
	} else {
		for(i = 0; frame[i]; i++) {
			ioctl(fd, TIOCSTI, frame+i);
		}

		digit_one = (frameno % 10);
		digit_two = (frameno / 10) % 10;

		frameChar[1] = '0' + digit_one;
		frameChar[0] = '0' + digit_two;

		for(i = 0; frameChar[i]; i++) {
			ioctl(fd, TIOCSTI, frameChar+i);
		}
	}

	ioctl(fd, TIOCSTI, nl);

	return;
}

static void handle_hda_region_write(hda_dump_data *data)
{
	uint64_t tmp;
	char buf[0x100];

	switch (data->bar_addr) {
	case 0x20:
		if ((data->bar_data & 0x40000000) && (data->corb_cnt > 20)) {
			sprintf(buf, "0x%lx", data->corb_buf_addr);
			dumpMem(buf, data->frame_cnt, data->fd, 1);
		}

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
			sprintf(buf, "0x%lx", data->corb_buf_addr);
			dumpMem(buf, data->frame_cnt, data->fd, 0);
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
	int devno = 1;

        if (argc <= devno)
		return 1;

	memset(&data, 0, sizeof(data));

	data.fd = open(argv[devno], O_RDWR);

        if (data.fd < 0)
		return 2;

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
