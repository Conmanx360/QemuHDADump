#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) 
{
	unsigned int corb_word;
	unsigned long rirb_word;
	int i, y, fd;
	char filename[256][16];
	char filename_sorted[256][128];
	unsigned int number_of_frames = 0;
	unsigned int array_size = 0;
	unsigned int found = 0;	
	unsigned int current_address = 0;	
	unsigned int print_address = 0;

	DIR *frame_directory;
	struct dirent *frame_file;
	char frame_no_str[2];
	char temp[7];
	char frame[7] = "frame \0";
	char file_no_temp[128];

	// Open the directory which the frame dumps are located in.
	
	if(argc != 2) {
		printf("Usage: %s folder_containing_framedumps\n", argv[0]);
		return 1;
	}
	else {
		frame_directory = opendir(argv[1]);
		if(frame_directory == NULL) {
			printf("Directory open failed!\n");
		}
	}

	// Check that the filename is actually a frame.

	i = 0;
	while((frame_file = readdir(frame_directory)) != NULL) {
		memcpy(temp, &frame_file->d_name[0], 5);
		if((strncmp(frame, temp, 5) == 0) && (strlen(frame_file->d_name) == 7)) { 
			strcpy(filename[i], frame_file->d_name);
			i++;
		}
	}	

	number_of_frames = i;

	// Sort the frames in numerical order.

	for(i = 0; i < number_of_frames; i++) {
		for(y = 0; (y < number_of_frames) && (found == 0); y++) {
			strcpy(file_no_temp, filename[y]);
			memcpy(frame_no_str, &file_no_temp[5], 2);

			if(atol(frame_no_str) == i) {
				strcpy(filename_sorted[i], filename[y]);
				found = 1;
			}	
		}
		found = 0;
	}

	// Allocate enough space to store the CORB verb data and RIRB response data.
//	printf("Number of frames * 256 = %i \n", (number_of_frames * 256));

	array_size = (number_of_frames * 256) * 4;

	unsigned int *corb = calloc(array_size, sizeof(unsigned int));
	unsigned long *rirb = calloc(array_size, sizeof(unsigned long));
	

//	for(i = 0; i < number_of_frames; i++) {
//		printf("%s \n", filename_sorted[i]);
//	}
	

	for(i = 0; i < number_of_frames; i++) {
		fd = open(filename_sorted[i], O_RDONLY);
		if(fd == -1) {
			printf("File open failed! \n");
		}		

		for(y = 0; y < 256; y++) {
			read(fd, &corb_word, sizeof(unsigned int));
			corb[current_address + y] = corb_word;
//			printf("CORB: 0x%08x \n", corb[current_address + y]);
		}

		lseek(fd, 0x800, SEEK_SET);		

		for(y = 0; y < 256; y++) {
			read(fd, &rirb_word, sizeof(unsigned long));
			rirb[current_address + y] = rirb_word;
//			printf("RIRB: 0x%lu \n", rirb[current_address + y]);
		}
		close(fd);
		
		current_address += 256;		
			
	}
	
//	printf("0x%08x: ", print_address);
//	for(i; i < number_of_frames * 256; i += 4) {
//		for(y = 0; y < 4; y++) {
//			printf("0x%08x ", corb[i + y]);
//		}
//		print_address += 16;
//		putchar('\n');
//		printf("0x%08x: ", print_address);
//	}

	int fd_corb = creat("allCORBframes", 0644);
	int fd_rirb = creat("allRIRBframes", 0644);	

	write(fd_corb, corb, array_size);
	write(fd_rirb, rirb, (array_size * 2));
	
	free(corb);
	free(rirb);

	close(fd_corb);
	close(fd_rirb);

}
