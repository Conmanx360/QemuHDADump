#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
   unsigned int word;
   ssize_t nr;
   int i = 0;
   unsigned int current_addr = 0;
   if(argc != 2) {
      printf("usage: %s filename\n", argv[0]);
   }
   else {
      // We assume argv[1] is a filename to open
      int fd = open(argv[1], O_RDONLY);
      if(fd == - 1) {
         printf("File open failed!\n");
      }

      printf("0x%08x: ", current_addr);
      while(read(fd, &word, sizeof(unsigned int)) != 0) {
         if(i < 4) {
            printf("0x%08x ", word);
            i++;
         }
         if(i == 4) {
            i = 0;
	    current_addr += 16;
            putchar('\n');
	    printf("0x%08x: ", current_addr);
         }
      }
   }
}
   
   

