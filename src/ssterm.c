// Ref. : https://en.wikibooks.org/wiki/Serial_Programming/termios
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h> // needed for memset

int main(int argc,char** argv)
{
	struct termios tio, old_tio;
	struct termios stdio, old_stdio;
	int tty_fd;
//	fd_set rdset;
	char tname[80] = "/dev/ttyO1"; // default terminal name.
//	unsigned char c='\0';
	ssize_t read_len;
	unsigned char read_data[256] = "\0";

	printf("Please start with %s /dev/ttyS1 (for example)\n",argv[0]);
	if (argc > 1)
	{
		if (strlen(argv[1]) >= sizeof(tname))
		{
			printf("Error: terminal name is too long!\r\n");
			exit(-1);
		}
		strncpy(tname, argv[1], sizeof(tname));
	}

	tcgetattr(STDIN_FILENO, &old_stdio);
	memset(&stdio,0,sizeof(stdio));
	stdio.c_iflag=0;
	stdio.c_oflag=0;
	stdio.c_cflag=0;
	stdio.c_lflag=0;
	stdio.c_cc[VMIN]=1;
	stdio.c_cc[VTIME]=0;
	tcsetattr(STDOUT_FILENO,TCSANOW,&stdio);
	tcsetattr(STDOUT_FILENO,TCSAFLUSH,&stdio);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);       // make the reads non-blocking

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag=0;
	tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
	tio.c_lflag=0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=5;

	tty_fd=open(tname, O_RDWR | O_NONBLOCK);
	if (tty_fd == 0)
	{
		printf("Error: open with %s.\r\n", tname);
		tcsetattr(STDIN_FILENO, TCSANOW, &old_stdio);
		exit(-1);
	}
	printf("Open terminal %s success!\r\n", tname);
	cfsetospeed(&tio,B115200);            // 115200 baud
	cfsetispeed(&tio,B115200);            // 115200 baud
	tcgetattr(tty_fd,&old_tio);
	tcsetattr(tty_fd,TCSANOW,&tio);

	while (read_data[0] != 'q')
	{
		// if new data is available on the serial port, print it out
		if ((read_len = read(tty_fd,read_data,sizeof(read_data))) > 0)
		{
			int i;

//			write(STDOUT_FILENO,read_data,read_len);
			dprintf(STDOUT_FILENO, "0x%x(%d): ", read_len, read_len);
			for (i = 0; i < read_len; i ++)
				dprintf(STDOUT_FILENO, "0x%x(%d)'%c' ", read_data[i], read_data[i], read_data[i]);
			dprintf(STDOUT_FILENO, "\r\n");
		}
		// if new data is available on the console, send it to the serial port
		if ((read_len = read(STDIN_FILENO,read_data,sizeof(read_len))) > 0)
		{
			write(tty_fd,read_data,read_len);
		}
	}

	tcsetattr(tty_fd,TCSANOW,&old_tio);
	close(tty_fd);
	tcsetattr(STDIN_FILENO, TCSANOW, &old_stdio);
	return 0;
}
