/*****************************************************************************
 wifi_se.c
  -Gateway system WiFi On/Off manage proc

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

// #define _DEBUG_
#ifdef _DEBUG_
	#define debug_print(...)	printf(__ARGV__)
#else
	#define debug_print(...)
#endif

#define GPIO_Wi_Fi_SW		"/sys/class/gpio/gpio26/value"
#define SW_MONITORING		3		/* 3s */

/******************************************************************************
 * NAME       : gpio_open
 * FUNCTION   : GPIO open
 * REMARKS    :
 *****************************************************************************/
static int gpio_open( void )
{
	int					gpio_fd = 0;

	gpio_fd = open( GPIO_Wi_Fi_SW, O_RDONLY );
	if( gpio_fd < 0 )
	{
		printf( "GPIO26 open error.\n" );
		return -1;
	}
	else
	{
		return gpio_fd;
	}
}

/******************************************************************************
 * NAME       : gpio_close
 * FUNCTION   : GPIO close
 * REMARKS    :
 *****************************************************************************/
static void gpio_close( int gpio_fd )
{
	close( gpio_fd );
}

/******************************************************************************
 * NAME       : io_value_get
 * FUNCTION   : I/O value get
 * REMARKS    :
 *****************************************************************************/
static int io_value_get( int gpio_fd, char *get_value )
{
	lseek( gpio_fd, 0, SEEK_SET );
	if( 0 < read( gpio_fd, get_value, 1 ) )
	{
		return 0;
	}
	else
	{
		printf( "GPIO26 read error.\n" );
		return -1;
	}
}


/******************************************************************************
 * NAME       : main
 * FUNCTION   : 
 * REMARKS    :
 *****************************************************************************/
int main( int argc, char **argv )
{
	char	*script;
	char	*command;
	int		gpio_fd;
	char	read_value = 0;
	int		sw_status = 1;	/* 0:WiFi ON / 1:WiFi OFF */
	int		sw_off_cnt = 0;
	int		sw_on_cnt = 0;

	/* wifi down */
	system("/sbin/ifconfig wlan0 down >/dev/null 2>&1");

	if( argc < 2 )
	{
		printf("require script name\n");
		return -1;
	}
	script = argv[1];

	/* max size "$script_name off" */
	command = malloc(strlen(script) + 5);
	if( command == NULL )
	{
		printf("malloc failed\n");
		return -1;
	}

	gpio_fd = gpio_open();
	if( -1 == gpio_fd )
	{
		free( command );
		return -1;
	}

	while(1)
	{
		if( 0 == io_value_get( gpio_fd, &read_value ) )
		{
			debug_print("GPIO = %c\n", read_value);
			if( 0x31 == read_value )
			{
				/* OFF */
				sw_on_cnt = 0;
				if( 0 == sw_status )	/* SW = ON */
				{
					sw_off_cnt++;
					debug_print("sw_off_cnt = %d\n", sw_off_cnt);
					if( sw_off_cnt >= SW_MONITORING )
					{
						/* OFF confirm */
						sw_status = 1;
						sprintf(command, "%s off", script);
						system(command);
						debug_print("wlan -> down\n");
					}
				}
			}
			else
			{
				/* ON */
				sw_off_cnt = 0;
				if( 1 == sw_status )	/* SW = OFF */
				{
					sw_on_cnt++;
					debug_print("sw_on_cnt = %d\n", sw_on_cnt);
					if( sw_on_cnt >= SW_MONITORING )
					{
						/* ON confirm */
						sw_status = 0;
						sprintf(command, "%s on", script);
						system(command);
						debug_print("wlan -> up\n");
					}
				}
			}
		}
		sleep(1);
	}

	gpio_close( gpio_fd );

	free( command );

	return 0;
}

