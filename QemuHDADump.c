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

void get_corb_buffer_addr(char *array, char *corb_buffer_addr, unsigned int tlo)
{
	unsigned int i;
	printf("CORB buffer Address:");
	for(i = 0; array[i + (tlo + 48)] != ','; i++) {
		corb_buffer_addr[i] = array[i + (tlo + 48)];
		printf("%c", corb_buffer_addr[i]);
	}
	putchar('\n');

	return;
}

unsigned int regionCheck(char *array, unsigned int tlo)
{
	return array[tlo + 40];

}

/*
 * Get the character offset from the PID and the time of the trace.
 */
int traceLineOffset(char *array)
{
	int i, j;
	char match_chars[] = { '@', '.', ':', 0 };
	i = j = 0;
	do {
	  for(;array[i] && array[i] != match_chars[j]; i++);
	  if (array[i] != match_chars[j])
	    return -1;
	  ++j;
	} while(match_chars[j] != 0);
	return i;
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

	for(i = 0; i < 10 && array[i]; i++) {
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

int main(int argc, char *argv[])
{
	char corb_buffer_location[16];
	unsigned short framenumber = 0;
	hda_dump_data data;
	int devno = 1;
	unsigned int i = 0;
	unsigned int total_verbs = 0;

        if (argc <= devno)
		return 1;

	memset(&data, 0, sizeof(data));

	data.fd = open(argv[devno], O_RDWR);

        if (data.fd < 0)
		return 2;

	data.buf_size = DEFAULT_BUF_SIZE;
	data.buf = calloc(data.buf_size, sizeof(*data.buf));

        memset(corb_buffer_location, 0, sizeof(corb_buffer_location));

	while(1) {
		int tlo = 0;  // trace line offset, due to PID
		unsigned short switch_check = 0;

		if (data.buf)
			memset(data.buf, 0, data.buf_size);
		fflush(stdout);
		if (getline(&data.buf, &data.buf_size, stdin) == -1)
		  break;

		tlo = traceLineOffset(data.buf);
		if (tlo < 0)
		  	// ignore non-trace lines
			continue;

		switch_check = regionCheck(data.buf, tlo);

		/* Check which PCI BAR region it is */
		switch(switch_check) {
		/* this is the HDA register region */
		case '0':
			switch (data.buf[tlo + 44]) {
			case '2':
				if (data.buf[tlo + 45] == '0') {
					if (data.buf[tlo + 50] == '4' && total_verbs > 20)
						dumpMem(corb_buffer_location, framenumber, data.fd, 1);
				}
				break;
			case '4':
				switch (data.buf[tlo + 45]) {
				case '0':
        				memset(corb_buffer_location, 0, sizeof(corb_buffer_location));
					get_corb_buffer_addr(data.buf, corb_buffer_location, tlo);
					break;
				case '8':
					total_verbs += 4;
					printf("0x%04x \n", total_verbs);
					if (data.buf[tlo + 50] == 'f' && data.buf[tlo + 51] == 'f') {
						dumpMem(corb_buffer_location, framenumber, data.fd, 0);
						framenumber++;
					}
					break;
				}
				break;
			case '8':
				if (data.buf[tlo + 45] != ',') {
					printf("Current verb 0x%04x Region0+", total_verbs);
					for (i = 0; data.buf[i + (tlo + 42)] != ')'; i++) {
						printf("%c", data.buf[i + (tlo + 42)]);
					}
				putchar('\n');
				}
				break;
			case '1':
				printf("Current verb 0x%04x Region0+", total_verbs);
				for (i = 0; data.buf[i + (tlo + 42)] != ')'; i++) {
					printf("%c", data.buf[i + (tlo + 42)]);
				}
				putchar('\n');
				break;
			}

			break;
		default:
			printf("Current verb 0x%04x tlo = %i Region%c+", total_verbs, tlo, switch_check);
			i = 0;
			while (data.buf[i + (tlo + 42)] != ')') {
				printf("%c", data.buf[i + (tlo + 42)]);
				i++;
			}
			putchar('\n');

			break;
		}

		if (data.buf)
			memset(data.buf, 0, data.buf_size);

		fflush(stdout);
	}

	if (data.buf)
		free(data.buf);

	return 0;
}
