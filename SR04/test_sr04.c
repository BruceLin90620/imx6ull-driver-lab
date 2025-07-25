
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>


int main(int argc, char **argv)
{
	int fd;
	int ns;

	int i;

	if (argc != 2) 
	{
		printf("Usage: %s <dev>\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1)
	{
		printf("can not open file %s\n", argv[1]);
		return -1;
	}


	while (1)
	{
		if (read(fd, &ns, 4) == 4)
		{
			printf("get distance: %d ns\n", ns);
			printf("get distance: %d mm\n", ns*340/2/1000000);  /* mm */
		}
		else
			printf("get distance: -1\n");
		sleep(1);
	}
	
	close(fd);
	
	return 0;
}


