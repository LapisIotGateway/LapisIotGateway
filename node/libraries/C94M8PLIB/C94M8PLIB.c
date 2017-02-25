
/* FILE NAME: c94m8plib.c
 *
 * Copyright (c) 2017  Lapis Semiconductor Co.,Ltd.
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "lazurite.h"
#include "c94m8plib.h"
#include "stdlib.h"

static HardwareSerial *c94m8p_port=NULL;

static struct s_c94m8p_settings c94m8p_settings;

void c94m8p_init(HardwareSerial* port,long baud)
{
	if(port!=NULL) c94m8p_port = port;
//	c94m8p_port->begin(baud);
	c94m8p_port->begin(baud);
}
void c94m8p_setRTK(unsigned char *data, int len)
{
	int wsize;
	int done = 0;
	do {
		wsize = c94m8p_port->write(data+done,len);
		done += wsize;
		len -= wsize;
	} while (len > 0);
}
static char work[128];

bool c94m8p_getGNRMC(char *gnrmc,int maxsize,unsigned long timeout)
{
	volatile unsigned long current_time;
	volatile unsigned long start_time;
	int data;
	int length;
	bool result=false;
	char *header;
	
	c94m8p_port->flush();
	start_time = millis();
	current_time = start_time;
	length = 0;
	do {
		data = c94m8p_port->read();
		if(data >= 0) {
			if(maxsize > length) {
				gnrmc[length] = (unsigned char) data;
			}
			if(data == '\n') {
				memcpy(work,gnrmc,sizeof(work));
				header = strtok(work,",\r\n");
				if(strcmp(header,"$GNRMC")==0) {
					gnrmc[length+1]=NULL;
					result = true;
					break;
				} else {
					length=0;
				}
			} else {
				length++;
			}
		}
		current_time=millis();
		digitalWrite(26,HIGH);
	} while (current_time < (start_time+timeout));
	
	return result;
}

void c94m8p_getMode(struct s_c94m8p_settings *mode)
{
	memcpy(&c94m8p_settings,mode,sizeof(c94m8p_settings));
}

void c94m8p_setMode(struct s_c94m8p_settings *mode)
{
	memcpy(mode,&c94m8p_settings,sizeof(c94m8p_settings));
}

const t_C94M8P c94m8p = {
	c94m8p_init,
	c94m8p_setRTK,
	c94m8p_getGNRMC,
	c94m8p_getMode,
	c94m8p_setMode
};

