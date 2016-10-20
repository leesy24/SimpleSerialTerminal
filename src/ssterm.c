// Ref. : https://en.wikibooks.org/wiki/Serial_Programming/termios
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#if __WIN32__
	#include <windows.h>
	#include <conio.h>
#elif __linux__
	#include <fcntl.h>
	#include <termios.h>
	#include <string.h> // needed for memset
#else
	#error "This program requires Linux or Win32."
#endif

#if __WIN32__
HANDLE hComm;                          // Handle to the Serial port
char com_name[80] = "COM4"; // default terminal name.

int ComOpen(char *port)
{
	BOOL  Status;                          // Status of the various operations

	if (port != NULL)
	{
		if (strlen(port) >= sizeof(com_name))
		{
			printf("Error: terminal name is too long!\r\n");
			return -1;
		}
		strncpy(com_name, port, sizeof(com_name));
	}

	printf("Opening com port %s~\r\n", com_name);

	/*---------------------------------- Opening the Serial Port -------------------------------------------*/

	hComm = CreateFile( com_name,        		      // Name of the Port to be Opened
                        GENERIC_READ | GENERIC_WRITE, // Read/Write Access
						0,                            // No Sharing, ports cant be shared
						NULL,                         // No Security
					    OPEN_EXISTING,                // Open existing port only
						FILE_ATTRIBUTE_NORMAL,        // Non Overlapped I/O
                        NULL);                        // Null for Comm Devices

	if (hComm == INVALID_HANDLE_VALUE)
	{
		printf("    Error! - Port %s can't be opened\r\n", com_name);
		return -1;
	}
	printf("    Port %s Opened\r\n", com_name);

	/*------------------------------- Setting the Parameters for the SerialPort ------------------------------*/

	DCB dcbSerialParams = { 0 };                         // Initializing DCB structure
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	Status = GetCommState(hComm, &dcbSerialParams);      //retreives  the current settings

	if (Status == FALSE)
	{
		printf("    Error! in GetCommState()\r\n");
		return -1;
	}

	dcbSerialParams.BaudRate = CBR_115200;      // Setting BaudRate = 115200
	dcbSerialParams.ByteSize = 8;             // Setting ByteSize = 8
	dcbSerialParams.StopBits = ONESTOPBIT;    // Setting StopBits = 1
	dcbSerialParams.Parity = NOPARITY;        // Setting Parity = None

	Status = SetCommState(hComm, &dcbSerialParams);  //Configuring the port according to settings in DCB

	if (Status == FALSE)
	{
		printf("    Error! in Setting DCB Structure\r\n");
		return -1;
	}
	printf("    Setting DCB Structure Successfull\r\n\r\n");
	printf("       Baudrate = %ld\r\n", dcbSerialParams.BaudRate);
	printf("       ByteSize = %d\r\n", dcbSerialParams.ByteSize);
	printf("       StopBits = %d\r\n", dcbSerialParams.StopBits);
	printf("       Parity   = %d\r\n", dcbSerialParams.Parity);

	/*------------------------------------ Setting Timeouts --------------------------------------------------*/

	COMMTIMEOUTS timeouts = { 0 };
#if 1 // Ref. https://groups.google.com/forum/#!topic/comp.os.ms-windows.programmer.win32/SotVc2_Eiig
	timeouts.ReadIntervalTimeout         = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant    = 0;
	timeouts.ReadTotalTimeoutMultiplier  = 0;
	timeouts.WriteTotalTimeoutConstant   = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
#else
	timeouts.ReadIntervalTimeout         = 50;
	timeouts.ReadTotalTimeoutConstant    = 50;
	timeouts.ReadTotalTimeoutMultiplier  = 10;
	timeouts.WriteTotalTimeoutConstant   = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
#endif

	if (SetCommTimeouts(hComm, &timeouts) == FALSE)
	{
		printf("    Error! in Setting Time Outs\r\n");
		return -1;
	}
	printf("    Setting Serial Port Timeouts Successfull\r\n");

	return 0;
}

int ComRead(void *buf, size_t nbytes)
{
	BOOL  Status;			// Status of the various operations
	DWORD NoBytesRead = 0;	// Bytes read by ReadFile()

	//printf("Reading com data!\r\n");
	Status = ReadFile(hComm, buf, nbytes, &NoBytesRead, NULL);
	if (Status == FALSE || NoBytesRead == 0)
	{
		//printf("Read zero byte com data!\r\n");
		return 0;
	}

	//printf("Read %ld byte com data!\r\n", NoBytesRead);
	return (int)NoBytesRead;
}

int ComWrite(const void *buf, size_t n)
{
	BOOL  Status;					// Status of the various operations
	DWORD dNoOfBytesWritten = 0;	// No of bytes written to the port

	//printf("Writing com data %d bytes!\r\n", n);
	Status = WriteFile(	hComm,				// Handle to the Serialport
						buf,				// Data to be written to the port
						n,					// No of bytes to write into the port
						&dNoOfBytesWritten,	// No of bytes written to the port
						NULL);

	if (Status == FALSE)
		return -1;

	//printf("Written com data!\r\n");
	return dNoOfBytesWritten;
}

int ComClose(void)
{
	CloseHandle(hComm);//Closing the Serial Port
	return 0;
}

int TermOpen(void)
{
	printf("Opening terminal~\r\n");
	printf("Open terminal success!\r\n");

	return 0;
}

int TermRead(void *buf, size_t nbytes)
{
	//printf("Reading terminal data!\r\n");
	char *data = buf;
	if (kbhit() == 0)
	{
		//printf("Read zero byte terminal data!\r\n");
		return 0;
	}
	data[0] = getch();
	//printf("Read one byte terminal data!\r\n");
	return 1;
}

int TermWrite(const void *buf, size_t n)
{
	int i;
	const char *data = buf;

	printf("0x%x(%d): ", n, n);
	for (i = 0; i < n; i ++)
		printf("0x%x(%d)'%c' ", data[i], data[i], data[i]);
	printf("\r\n");

	return n;
}

int TermClose(void)
{
	printf("Closing terminal~\r\n");
	printf("Close terminal success!\r\n");
	return 0;
}
#endif
#if __linux__
struct termios tio, old_tio;
struct termios stdio, old_stdio;
int tty_fd;
char com_name[80] = "/dev/ttyO1"; // default terminal name.

int ComOpen(char *port)
{
	printf("Opening com port %s(0x%x)\r\n", port?port:"NULL", (unsigned int)port);
	if (port != NULL)
	{
		if (strlen(port) >= sizeof(com_name))
		{
			printf("Error: terminal name is too long!\r\n");
			return -1;
		}
		strncpy(com_name, port, sizeof(com_name));
	}

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag=0;
	tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
	tio.c_lflag=0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=5;

	tty_fd=open(com_name, O_RDWR | O_NONBLOCK);
	if (tty_fd == 0)
	{
		printf("Error: open with %s.\r\n", com_name);
		tcsetattr(STDIN_FILENO, TCSANOW, &old_stdio);
		exit(-1);
	}
	printf("Open com port %s success!\r\n", com_name);
	cfsetospeed(&tio,B115200);            // 115200 baud
	cfsetispeed(&tio,B115200);            // 115200 baud
	tcgetattr(tty_fd,&old_tio);
	tcsetattr(tty_fd,TCSANOW,&tio);

	return 0;
}

int ComRead(void *buf, size_t nbytes)
{
	return read(tty_fd, buf, nbytes);
}

int ComWrite(const void *buf, size_t n)
{
	return write(tty_fd, buf, n);
}

int ComClose(void)
{
	tcsetattr(tty_fd,TCSANOW,&old_tio);
	close(tty_fd);
	return 0;
}

int TermOpen(void)
{
	printf("Opening terminal~\r\n");
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
	printf("Open terminal success!\r\n");

	return 0;
}

int TermRead(void *buf, size_t nbytes)
{
	return read(STDIN_FILENO, buf, nbytes);
}

int TermWrite(const void *buf, size_t n)
{
	int i;
	const char *data = buf;

	//write(STDOUT_FILENO,read_data,read_len);
	dprintf(STDOUT_FILENO, "0x%x(%d): ", n, n);
	for (i = 0; i < n; i ++)
		dprintf(STDOUT_FILENO, "0x%x(%d)'%c' ", data[i], data[i], data[i]);
	dprintf(STDOUT_FILENO, "\r\n");

	return n;
}

int TermClose(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_stdio);
	return 0;
}
#endif

int main(int argc, char** argv)
{
	ssize_t read_len;
	unsigned char read_data[256] = "\0";

	printf("Please start with %s /dev/ttyS1 or COM4 (for example)\r\n",argv[0]);
	printf("argc = %d\r\n", argc);
	printf("argv[1] = 0x%x\r\n", (unsigned int)argv[1]);

	if (TermOpen())
	{
		exit(-1);
	}

	if (ComOpen(argc > 1 ? argv[1] : NULL))
	{
		TermClose();
		exit(-1);
	}

	while (read_data[0] != 0x1b && read_data[0] != 'q')
	{
		// if new data is available on the serial port, print it out
		if ((read_len = ComRead(read_data, sizeof(read_data))) > 0)
		{
			TermWrite(read_data, read_len);
		}
		// if new data is available on the console, send it to the serial port
		if ((read_len = TermRead(read_data, sizeof(read_len))) > 0)
		{
			ComWrite(read_data, read_len);
		}
	}

	ComClose();
	TermClose();
	return 0;
}
