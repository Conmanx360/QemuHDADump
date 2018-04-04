#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


#define MAXLINE 128
#define REGION_ZERO 1
#define REGION_TWO  2

void get_corb_buffer_addr(char *array, char *corb_buffer_addr, unsigned int tlo)
{
	unsigned int i;

	for(i = 0; array[i + (tlo + 48)] != ','; i++) {
		corb_buffer_addr[i] = array[i + (tlo + 48)];
		printf("%c", corb_buffer_addr[i]);
	}
	putchar('\n');

	return;
}

unsigned int regionCheck(char *array, unsigned int tlo)
{
	unsigned int current_region = 0;
//	printf("region check entered \n");	
//	printf("Char region: %c \n", array[tlo + 40]);
	switch(array[tlo + 40]) {
	case '0':
//		printf("region zero detected. \n");
		return 1;
	case '2':
//		printf("region two detected. \n");
		return 2;
	default:
		return 0;
	}
	
}

/*
 * Get the character offset from the PID and the time of the trace.
 */
unsigned int traceLineOffset(char *array) 
{
	unsigned int tlo, i;
//	printf("traceLineOffset entered.\n");
	for(i = 0; array[i] != ':'; i++) {
		tlo = i;
	}

	return tlo + 1;
}

/*
 * Get the trace line.
 */
//Should you base it on paranthese ')' instead of newline?
//Would save you from problems when newline doesn't occur...
void getLine(char *array) 
{
	char c;
	unsigned short i = 0;

	memset(&array[0], 0, sizeof(array));
//	printf("getLine enter. \n");	
	while((c=getchar()) != ')') {
		array[i] = c;
		i++;
	}
	array[i + 1] = ')';
//	printf("getLine exit. \n");	
 
	return;
}


void dumpMem(char *array, unsigned short framenumber, int fd) 
{
	int i;
	int file = fd;
	unsigned short frameno = framenumber;
	unsigned int digit_one, digit_two;
  
	char nl[] = "\n";
	char stop[] = "stop\n";
	char cont[] = "cont\n";
	char pmemsave_part1[] = "pmemsave ";
	char pmemsave_part2[] = " 0x1000 ";
	char frame[] = "frame";
	char frameChar[] = "00"; 
   
	printf("dumpMem entered...\n");   

	for(i = 0; pmemsave_part1[i]; i++) {
		ioctl(fd, TIOCSTI, pmemsave_part1+i);
	}

	for(i = 0; i < 10; i++) {
		ioctl(fd, TIOCSTI, array+i);
	}

	for(i = 0; pmemsave_part2[i]; i++) {
		ioctl(fd, TIOCSTI, pmemsave_part2+i);
	}

	for(i = 0; frame[i]; i++) {
		ioctl(fd, TIOCSTI, frame+i);
	}

	digit_one = (frameno % 10);
	digit_two = (frameno / 10);
	
	frameChar[1] = '0' + digit_one;
	frameChar[0] = '0' + digit_two;
 
	for(i = 0; frameChar[i]; i++) {
		ioctl(fd, TIOCSTI, frameChar+i);
	}
   
	ioctl(fd, TIOCSTI, nl);

	return;
}

int main(int argc, char *argv[]) 
{
	unsigned short framenumber = 0;
	int devno = 1;
	int fd;
	int i;
	unsigned int total_verbs = 0;
	char cont[] = "cont\n";   
 
	fd = open(argv[devno], O_RDWR);
    
	for(i = 0; cont[i]; i++) {
		ioctl(fd, TIOCSTI, cont+i);
	}

	while(1) {
		char corb_buffer_location[16];	   
		unsigned short tlo = 0;  // trace line offset, due to PID
		char trace_line[MAXLINE];
		unsigned short switch_check = 0;
		unsigned short i;   
		unsigned short init_array = 0;		

		getLine(trace_line);
		tlo = traceLineOffset(trace_line);
		switch_check = regionCheck(trace_line, tlo);
//		printf("Switch check = %d \n", switch_check); 
		if(switch_check == 1) {
			switch(trace_line[tlo + 44]) {
			case '4':
//				printf("case 4 entered. \n");
				switch(trace_line[tlo + 45]) {
				case '0':
					get_corb_buffer_addr(trace_line, corb_buffer_location, tlo);
					break;
				case '8':
					total_verbs += 4;
					printf("0x%04x \n", total_verbs);
					if(trace_line[tlo + 50] == 'f' && trace_line[tlo + 51] == 'f') {
						dumpMem(corb_buffer_location, framenumber, fd);
						framenumber++;
					}
					break;
				}
				break;
			case '8':
				if(trace_line[tlo + 45] != ',') {
					printf("Current verb 0x%04x Region0+", total_verbs);
					for(i = 0; trace_line[i + (tlo + 42)] != ')'; i++) {
						printf("%c", trace_line[i + (tlo + 42)]);
					}
				putchar('\n');
				}
				break;
			case '1':
				printf("Current verb 0x%04x Region0+", total_verbs);
				for(i = 0; trace_line[i + (tlo + 42)] != ')'; i++) {
					printf("%c", trace_line[i + (tlo + 42)]);
				}
				putchar('\n');
				break;
			}
		}
		else if(switch_check == 2) {
			printf("Current verb 0x%04x Region2+", total_verbs);
			i = 0;
			while(trace_line[i + (tlo + 42)] != ')') {
				printf("%c", trace_line[i + (tlo + 42)]);
				i++;
			}
//			for(i = 0; trace_line[i + (tlo + 42)] != '\n'; i++) {
//				printf("%c", trace_line[i + (tlo + 42)]);
//			}
			putchar('\n');
		}
		memset(&trace_line[0], 0, sizeof(trace_line));
	}
}



