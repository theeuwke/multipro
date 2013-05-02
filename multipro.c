#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

/* we need a termios structure to clear the HUPCL bit */
struct termios tio;

/* original api code (delphi, jikes!)
procedure SendInstruction(Command,SubCommand:Byte);
var Str:string;
I:integer;
Ch,Chksm:Char;
begin
Str:='';
//Start with 10 times Chr(21)
for I:=1 to 10 do Str:=Str+Chr(21);
//Select the Command
Str:=Str+Chr(Command);
//Select the Sub-Command
Str:=Str + Chr(SubCommand);
//Begin checksum is 0
Chksm:=#$00;
//Calculate the checksum of the instruction string
For I:=1 to Length(Str) do
Chksm:=Chr(CheckSum(Ord(Chksm),Ord(Str[I])));
Str:=Str+Chksm;
//Open the comport and set the RTS and DTR line in high state
Form1.Comport1.Open;
Form1.Comport1.SetRTS(True);
Form1.ComPort1.SetDTR(True);
//Send the instruction string
For I:=1 to Length(Str) do
begin
Ch:=Str[I];
delay(5);
Form1.Comport1.Write(Ch,1,True);
end;
//close the com-port
Form1.Comport1.Close;
end;
*/

int main(int argc, char *argv[])
{
	int fd;
	int status;
	int i;
	unsigned char chksum, prev, result;
	unsigned char string[13];
	char mode;

	if ( (argc != 3) || ( atoi(argv[2]) < 0) || (atoi(argv[2]) > 7) )
	{
		printf("Usage: multipro port mode\n");
		printf("Usage: multipro /dev/ttyS0|/dev/ttyS1 0|1|2|3|4|5|6|7\n");
		printf("00 Idle mode, Programmer in idle state --> No power on smartcard\n");
		printf("01 Phoenix 3.57MHz\n");
		printf("02 Phoenix 6.00MHz\n");
		printf("03 Smartmouse 3.57MHz\n");
		printf("04 Smartmouse 6.00MHz\n");
		printf("05 PonyProg 3.57MHz (Default)\n");
		printf("06 PonyProg 6.00MHz\n");
		printf("07 JDM emulation\n");
		exit( 1 );
	}

	if ((fd = open(argv[1],O_RDWR)) < 0)
	{
		printf("Couldn't open %s\n",argv[1]);
		exit(1);
	}
	tcgetattr(fd, &tio);          /* get the termio information */
	tio.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
	tio.c_cflag &= ~HUPCL;        /* clear the HUPCL bit */
	tcsetattr(fd, TCSANOW, &tio); /* set the termio information */

	ioctl(fd, TIOCMGET, &status); /* get the serial port status */

	/* start with 10x '21' */
	for(i = 0; i < 10; i++) {
		string[i] = 21;
	}
	/* put the command in (00) */
	string[10] = 0;
	/* put the sub-command in (04) */
	string[11] = atoi(argv[2]);
	/* calculate checksum over the data */
	chksum = 0;
	printf("Calculating checksum...\n");
	for(i = 0; i < 12; i++) {
		result = chksum ^ string[i];
		if((result & 128) == 128)
			result <<= 1;
		else
			result = (result << 1) + 1;
		chksum = result;
	}
	/* put checksum in the string */
	string[12] = chksum;

	/* check the string */
	printf("Characters to send\n");
	for(i = 0; i < 13; i++) {
		printf("%02x ", (unsigned char) string[i]);
	}
	printf("\n");

	/* set DTR & RTS */
	status |= TIOCM_DTR;
	status |= TIOCM_RTS;

	ioctl(fd, TIOCMSET, &status); /* set the serial port status */
	result = 0;
	/* write command */
	for(i = 0; i < 13; i++) {
		result += write(fd, &string[i], 1);
		usleep(5000);
	}
	if (result < 0)
	{
		fputs("write failed\n", stderr);
		close(fd);
		exit(1);
	}
	printf("%d bytes written\n", result);
	close(fd);                    /* close the device file */
}
