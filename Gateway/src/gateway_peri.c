/*****************************************************************************
 gateway_peri.c
  -Gateway system peripheral initial proc

 Copyright (C) 2017 Lapis Semiconductor Co., LTD.
 All rights reserved.

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>


//#define _DEBUG_
#ifdef _DEBUG_
	#define debug_print(fmt,...)		printf(fmt ## __VA_ARGS__)
#else
	#define debug_print(fmt,...)
#endif

typedef struct {
	char	port[4];	/* GPIO port# */
	char	data[4];	/* out/in */
}GATEWAY_GPIO_TBL;

/* WP				: GPIO4  (O)	*/
/* POWER_OFF_N		: GPIO5  (O)	*/
/* REGPDIN			: GPIO6  (O)	*/
/* VCC_LTE_Enable	: GPIO12 (O)	*/
/* CBS_RI_N			: GPIO13 (I)	*/
/* USB_Disable		: GPIO16 (O)	*/
/* A2				: GPIO17 (O)	*/
/* BP3596A_SINTN	: GPIO18 (I)	*/
/* RMT_CTRL_IND		: GPIO19 (I)	*/
/* PS_HOLD			: GPIO20 (I)	*/
/* BOOT_IND			: GPIO21 (I)	*/
/* RM92A_RESETn		: GPIO22 (O)	*/
/* POWER_ON			: GPIO23 (O)	*/
/* WAKE_ON_WWAN_N	: GPIO24 (I)	*/
/* Wi_Fi_SW			: GPIO26 (I)	*/
/* RESETN			: GPIO27 (O)	*/
#define GPIO_WP				"4"
#define GPIO_POWER_OFF_N	"5"
#define GPIO_REGPDIN		"6"
#define GPIO_VCC_LTE_Enable	"12"
#define GPIO_CBS_RI_N		"13"
#define GPIO_USB_Disable	"16"
#define GPIO_A2				"17"
#define GPIO_BP3596A_SINTN	"18"
#define GPIO_RMT_CTRL_IND	"19"
#define GPIO_PS_HOLD		"20"
#define GPIO_BOOT_IND		"21"
#define GPIO_RM92A_RESETn	"22"
#define GPIO_POWER_ON		"23"
#define GPIO_WAKE_ON_WWAN_N	"24"
#define GPIO_Wi_Fi_SW		"26"
#define GPIO_RESETN			"27"

#define	GPIO_IN		"in"
#define	GPIO_OUT	"out"

GATEWAY_GPIO_TBL	gpio_export_direction_tbl[] = {
	GPIO_WP,				GPIO_OUT,
	GPIO_POWER_OFF_N,		GPIO_OUT,
	GPIO_REGPDIN,			GPIO_OUT,
	GPIO_VCC_LTE_Enable,	GPIO_OUT,
	GPIO_CBS_RI_N,			GPIO_IN,
	GPIO_USB_Disable,		GPIO_OUT,
	GPIO_A2,				GPIO_OUT,
	GPIO_BP3596A_SINTN,		GPIO_IN,
	GPIO_RMT_CTRL_IND,		GPIO_IN,
	GPIO_PS_HOLD,			GPIO_IN,
	GPIO_BOOT_IND,			GPIO_IN,
	GPIO_RM92A_RESETn,		GPIO_OUT,
	GPIO_POWER_ON,			GPIO_OUT,
	GPIO_WAKE_ON_WWAN_N,	GPIO_IN,
	GPIO_Wi_Fi_SW,			GPIO_IN,
	GPIO_RESETN,			GPIO_OUT,
	0, 0
};

GATEWAY_GPIO_TBL	gpio_init_tbl[] = {
	GPIO_WP,				"0",
	GPIO_A2,				"0",
	GPIO_RESETN,			"0",
	GPIO_RM92A_RESETn,		"0",
	GPIO_POWER_OFF_N,		"1",
	GPIO_REGPDIN,			"1",
	GPIO_POWER_ON,			"0",
	GPIO_VCC_LTE_Enable,	"0",
	GPIO_USB_Disable,		"0",
	0, 0
};

int	SHUTDOWN_FLAG = 0;


#define BAUDRATE B115200 
#define MODEMDEVICE "/dev/ttyUSB1"



/******************************************************************************
 * NAME       : io_export
 * FUNCTION   : I/O initialize
 * REMARKS    :
 *****************************************************************************/
static int io_export( void )
{
	int					loop;
	int					exp_fd = 0;
	int					io_fd = 0;
	char				direction[64];
	GATEWAY_GPIO_TBL	*pTbl;

	/* I/O export */
	exp_fd = open( "/sys/class/gpio/export", O_WRONLY );
	if( exp_fd < 0 )
	{
		printf( "GPIO export open error.\n" );
		goto ErrEnd;
	}

	for( loop=0 ;; loop++ )
	{
		pTbl = &gpio_export_direction_tbl[loop];

		if( pTbl->port[0] )
		{
			write( exp_fd, pTbl->port, strlen(pTbl->port)+1 );
		}
		else
		{
			break;
		}

		/* 100ms wait */
		usleep(100000);

		sprintf( direction, "/sys/class/gpio/gpio%s/direction", pTbl->port );
		io_fd = open( direction, O_WRONLY );
		if( io_fd < 0 )
		{
			printf( "GPIO%s direction open error.\n", pTbl->port );
			goto ErrEnd;
		}
		write( io_fd, pTbl->data, strlen(pTbl->data)+1 );
		close( io_fd );
	}
	close( exp_fd );

	return 0;

ErrEnd :
	if( exp_fd ) close( exp_fd );
	if( io_fd ) close( io_fd );

	return -1;
}


/******************************************************************************
 * NAME       : io_unexport
 * FUNCTION   : I/O unexport
 * REMARKS    :
 *****************************************************************************/
static void io_unexport( void )
{
	int					loop;
	int					exp_fd = 0;
	GATEWAY_GPIO_TBL	*pTbl;

	/* I/O export */
	exp_fd = open( "/sys/class/gpio/unexport", O_WRONLY );
	if( exp_fd < 0 )
	{
		printf( "GPIO unexport open error.\n" );
		return;
	}

	for( loop=0 ;; loop++ )
	{
		pTbl = &gpio_export_direction_tbl[loop];

		if( pTbl->port[0] )
		{
			write( exp_fd, pTbl->port, strlen(pTbl->port)+1 );
		}
		else
		{
			break;
		}
	}
	close( exp_fd );

	return;
}


/******************************************************************************
 * NAME       : io_init
 * FUNCTION   : I/O init
 * REMARKS    :
 *****************************************************************************/
static int io_init( void )
{
	int					loop;
	int					gpio_fd = 0;
	char				value[64];
	GATEWAY_GPIO_TBL	*pTbl;

	/* set initial value */
	for( loop=0 ;; loop++ )
	{
		pTbl = &gpio_init_tbl[loop];

		if( pTbl->port[0] )
		{
			sprintf( value, "/sys/class/gpio/gpio%s/value", pTbl->port );
			gpio_fd = open( value, O_WRONLY );
			if( gpio_fd < 0 )
			{
				printf( "GPIO%s value open error.\n", pTbl->port );
				goto ErrEnd;
			}
			write( gpio_fd, pTbl->data, strlen(pTbl->data)+1 );
			close( gpio_fd );
		}
		else
		{
			break;
		}
	}
	return 0;

ErrEnd :
	return -1;
}


/******************************************************************************
 * NAME       : io_value_set
 * FUNCTION   : I/O value set
 * REMARKS    :
 *****************************************************************************/
static int io_value_set( char *gpio, char *set_value )
{
	int	gpio_fd = 0;
	char				value[64];

	sprintf( value, "/sys/class/gpio/gpio%s/value", gpio );
	gpio_fd = open( value, O_WRONLY );
	if( gpio_fd < 0 )
	{
		printf( "GPIO%s value open error.\n", gpio );
		return -1;
	}
	write( gpio_fd, set_value, strlen(set_value)+1 );
	close( gpio_fd );

	return 0;
}


/******************************************************************************
 * NAME       : io_value_get
 * FUNCTION   : I/O value get
 * REMARKS    :
 *****************************************************************************/
static int io_value_get( char *gpio, char *get_value )
{
	int		gpio_fd = 0;
	char				value[64];

	sprintf( value, "/sys/class/gpio/gpio%s/value", gpio );
	gpio_fd = open( value, O_RDONLY );
	if( gpio_fd < 0 )
	{
		printf( "GPIO%s value open error.\n", gpio );
		return -1;
	}
	read( gpio_fd, get_value, 1 );
	close( gpio_fd );

	return 0;
}

/******************************************************************************
 * NAME       : signal_handler
 * FUNCTION   : receive signal
 * REMARKS    :
 *****************************************************************************/
static void signal_handler( int sig )
{
	printf("signal_handler receive :%d.\n", sig);

	/* system shutdown */
	SHUTDOWN_FLAG = 0;
}

/******************************************************************************
 * NAME       : shutdown_ATcommand
 * FUNCTION   : shutdown at command
 * REMARKS    :
 *****************************************************************************/
static int shutdown_ATcommand( void )
{
	int modem_fd;
	struct termios oldtio,newtio;

	modem_fd = open( MODEMDEVICE, O_RDWR | O_NOCTTY );
	if( modem_fd < 0 )
	{
		perror( MODEMDEVICE );
		return -1;
	}

	tcgetattr( modem_fd, &oldtio );
	bzero( &newtio, sizeof(newtio) );

	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR | ICRNL;
	newtio.c_oflag = 0;
	newtio.c_lflag = ICANON;

	newtio.c_cc[VINTR] = 0;		/* Ctrl-c */ 
	newtio.c_cc[VQUIT] = 0;		/* Ctrl-\ */
	newtio.c_cc[VERASE] = 0;	/* del */
	newtio.c_cc[VKILL] = 0;		/* @ */
	newtio.c_cc[VEOF] = 4;		/* Ctrl-d */
	newtio.c_cc[VTIME] = 0;		/* キャラクタ間タイマを使わない */
	newtio.c_cc[VMIN] = 1;		/* 1文字来るまで，読み込みをブロックする */
	newtio.c_cc[VSWTC] = 0;		/* '\0' */
	newtio.c_cc[VSTART] = 0;	/* Ctrl-q */ 
	newtio.c_cc[VSTOP] = 0;		/* Ctrl-s */
	newtio.c_cc[VSUSP] = 0;		/* Ctrl-z */
	newtio.c_cc[VEOL] = 0;		/* '\0' */
	newtio.c_cc[VREPRINT] = 0;	/* Ctrl-r */
	newtio.c_cc[VDISCARD] = 0;	/* Ctrl-u */
	newtio.c_cc[VWERASE] = 0;	/* Ctrl-w */
	newtio.c_cc[VLNEXT] = 0;	/* Ctrl-v */
	newtio.c_cc[VEOL2] = 0;		/* '\0' */

	tcflush( modem_fd, TCIFLUSH );
	tcsetattr( modem_fd, TCSANOW, &newtio );

	/* shutdown command */
	write( modem_fd ,"at+ifc=0,0\r\n", 12);
	sleep(1);
	write( modem_fd ,"at*reset=11\r\n", 13);

	close( modem_fd );

	return 0;
}

/******************************************************************************
 * NAME       : main
 * FUNCTION   : 
 * REMARKS    :
 *****************************************************************************/
int main( int argc, char **argv )
{
	char	read_value;
	int		loop;
	struct sigaction	sa;
#ifdef _DEBUG_
	FILE *debug_fd;
#endif

	/* regist signal handler */
	memset( &sa, 0, sizeof(sa) );
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = signal_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	SHUTDOWN_FLAG = 1;

	/* I/O initialize */
	if( 0 != io_export() )
	{
		printf( "I/O export/direction error.\n" );
		io_unexport();
		exit(1);
	}
	debug_print( "I/O export/direction OK.\n" );
	if( 0 != io_init() )
	{
		printf( "I/O initialite error.\n" );
		io_unexport();
		exit(1);
	}
	debug_print( "I/O initialite OK.\n" );

	/* MM-M500 reset proc */
	debug_print( "MM-M500 reset proc start.\n" );
	if( 0 == io_value_set( GPIO_VCC_LTE_Enable, "1" ) )
	{
		debug_print( "MM-M500 reset proc GPIO_VCC_LTE_Enable high OK .\n" );
		if( 0 == io_value_set( GPIO_POWER_OFF_N, "0" ) )
		{
			debug_print( "MM-M500 reset proc GPIO_POWER_OFF_N low OK .\n" );
			if( 0 == io_value_set( GPIO_POWER_ON, "1" ) )
			{
				debug_print( "MM-M500 reset proc GPIO_POWER_ON high OK .\n" );

				for( loop=0 ; loop<10 ; loop++ )
				{
					/* 200ms wait */
					usleep(200000);

					if( 0 != io_value_get( GPIO_PS_HOLD, &read_value ) )
					{
						printf( "MM-M500 reset error(GPIO%s) read.\n",GPIO_PS_HOLD );
						break;
					}
					if( 0x31 == read_value )
					{
						debug_print( "MM-M500 reset OK.\n" );
						break;
					}
				}
				if( loop==10 )
				{
					printf( "MM-M500 reset GPIO_PS_HOLD ->high timeout.\n" );
				}
				if( 0 != io_value_set( GPIO_POWER_ON, "0" ) )
				{
					printf( "MM-M500 reset error(GPIO%s) low.\n",GPIO_POWER_ON );
				}
			}
			else printf( "MM-M500 reset error(GPIO%s) high.\n",GPIO_POWER_ON );
		}
		else printf( "MM-M500 reset error(GPIO%s) low.\n",GPIO_POWER_OFF_N );
	}
	else printf( "MM-M500 reset error(GPIO%s) high.\n",GPIO_VCC_LTE_Enable );

	/* RM92A reset proc */
	debug_print( "RM92A reset proc start.\n" );
	if( 0 == io_value_set( GPIO_RM92A_RESETn, "0" ) )
	{
		/* 10ms wait */
		usleep(10000);
		if( 0 == io_value_set( GPIO_RM92A_RESETn, "1" ) )
		{
			debug_print( "RM92A reset fin.\n" );
		}
		else printf( "RM92A reset error(GPIO%s) high.\n",GPIO_RM92A_RESETn );
	}
	else printf( "RM92A reset error(GPIO%s) low.\n",GPIO_RM92A_RESETn );

	/* wait shutdown signal */
	while( SHUTDOWN_FLAG )
	{
		sleep(1);
	}

	system("sudo /sbin/modprobe -r cp210x >/dev/null 2>&1");
	sleep(2);
	system("sudo /sbin/modprobe -r ftdi_sio >/dev/null 2>&1");
	sleep(5);
	system("sudo /sbin/modprobe usbserial vendor=0x2a9e product=0x0103 >/dev/null 2>&1");
	sleep(3);
	// one more
	system("sudo /sbin/modprobe usbserial vendor=0x2a9e product=0x0103 >/dev/null 2>&1");

#ifdef _DEBUG_
	debug_fd = fopen("/home/pi/gateway/shutdown_debug.txt", "a");
#endif

	/* shutdown proc */
	debug_print( "MM-M500 shutdown proc start.\n" );
#ifdef _DEBUG_
	fputs( "MM-M500 shutdown proc start.\n", debug_fd );
#endif
	if( 0 == io_value_set( GPIO_POWER_ON, "0" ) )
	{
		/* 100ms wait */
		usleep(100000);
		/* AT command(shutdown) */
		debug_print( "MM-M500 shutdown AT command.\n" );
#ifdef _DEBUG_
		fputs( "MM-M500 shutdown AT command.\n", debug_fd );
#endif
		if( 0 == shutdown_ATcommand() )
		{
			/* GPIO_PS_HOLD=low check max 30s */
			for( loop=0 ; loop<155 ; loop++ )
			{
				/* 200ms wait */
				usleep(200000);

				if( 0 != io_value_get( GPIO_PS_HOLD, &read_value ) )
				{
					printf( "MM-M500 shutdown error(GPIO%s) read.\n",GPIO_PS_HOLD );
#ifdef _DEBUG_
					fputs( "MM-M500 shutdown error gpio read.\n", debug_fd );
#endif
					break;
				}
				if( 0x30 == read_value )
				{
					debug_print( "MM-M500 shutdown GPIO_PS_HOLD OK.\n" );
#ifdef _DEBUG_
					fputs( "MM-M500 shutdown GPIO_PS_HOLD OK.\n", debug_fd );
#endif
					break;
				}
			}
			if( loop==155 )
			{
				printf( "MM-M500 shutdown GPIO_PS_HOLD ->low timeout.\n" );
#ifdef _DEBUG_
				fputs( "MM-M500 shutdown GPIO_PS_HOLD ->low timeout.\n", debug_fd );
#endif
				exit(1);
			}
			printf( "MM-M500 shutdown GPIO_PS_HOLD loop=%d.\n",loop );
#ifdef _DEBUG_
			{
				char work[128];
				
				sprintf( work, "MM-M500 shutdown GPIO_PS_HOLD loop=%d.\n", loop );
				fputs( work, debug_fd );
			}
#endif

			/* 6000ms wait */
			usleep(600000);
			debug_print( "MM-M500 shutdown proc wait 600ms GPIO_POWER_OFF_N ->high.\n" );
#ifdef _DEBUG_
			fputs( "MM-M500 shutdown proc wait 600ms GPIO_POWER_OFF_N ->high.\n", debug_fd );
#endif
			if( 0 == io_value_set( GPIO_POWER_OFF_N, "1" ) )
			{
				debug_print( "MM-M500 shutdown proc finish.\n" );
#ifdef _DEBUG_
				fputs( "MM-M500 shutdown proc finish.\n", debug_fd );
#endif
			}
			else printf( "MM-M500 shutdown error(GPIO%s) set high.\n",GPIO_POWER_OFF_N );
		}
		else printf( "MM-M500 shutdown at command error.\n");
	}
	else printf( "MM-M500 shutdown error(GPIO%s) low.\n",GPIO_POWER_ON );

#ifdef _DEBUG_
	fclose(debug_fd);
#endif

	return 0;
}

