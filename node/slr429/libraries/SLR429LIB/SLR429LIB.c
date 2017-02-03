
/* FILE NAME: slr429.c
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
#include "slr429lib.h"

//#define DEBUG_LORA

static unsigned char lora_rx_buf[256];
static int lora_rx_write_p;
static int lora_rx_packet_length;
static int lora_rx_packet_offset;
static bool lora_rx_read_lock;


static t_SLR429_CONFIG slr429_config;
static HardwareSerial* slr429_port;

static int wait_msg_timeout(char* msg, unsigned long limit_time)
{
	volatile unsigned long current_time;
	unsigned long start_time;
	int data;
	int result = -1;
	lora_rx_write_p = 0;
	start_time = millis();

	do
	{
		if(slr429_port->available()>0)
		{
			data = slr429_port->read();
			lora_rx_buf[lora_rx_write_p] = (unsigned char)data;
			lora_rx_write_p++;
#ifdef	DEBUG_LORA
			Serial.write_byte((unsigned char)data);
#endif
			if(data == '\n')
			{
				strtok(lora_rx_buf,"\r\n");
				if(msg != NULL) {
					if(strncmp(lora_rx_buf,msg,sizeof(lora_rx_buf)) == 0)
					{
						result = 0;
						lora_rx_write_p = 0;
						break;
					} else {
						lora_rx_write_p = 0;
					}
				} else {
					strtok(lora_rx_buf,"=\r\n");
					if(strncmp(lora_rx_buf,"*ER",sizeof(lora_rx_buf)) == 0)
					{
						result = -1;
					} else {
						result = 0;
					}
					lora_rx_write_p = 0;
					break;
				}
			}
		}
		current_time = millis();
	} while(current_time < (start_time+limit_time));
	
	return result;
}

static int slr429_init(HardwareSerial* port,t_SLR429_CONFIG *config)
{
	int result=0;
	
	if(port) slr429_port = port;
	else {
		result = -1;
		goto error;
	}
	
	port->begin(19200L);

	memcpy(&slr429_config,config,sizeof(t_SLR429_CONFIG));
	
	pinMode(slr429_config.rts,INPUT);
	pinMode(slr429_config.cts,OUTPUT);
	pinMode(slr429_config.ini,OUTPUT);
	pinMode(slr429_config.rstb,OUTPUT);
	
	digitalWrite(slr429_config.rts,LOW);
	digitalWrite(slr429_config.ini,HIGH);
	digitalWrite(slr429_config.rstb,LOW);
	delay(100);
	digitalWrite(slr429_config.rstb,HIGH);
	delay(100);
	digitalWrite(slr429_config.ini,LOW);
	delay(4000);
	digitalWrite(slr429_config.ini,HIGH);
	
	result = wait_msg_timeout("*RESET",3000);
	result = wait_msg_timeout(NULL,3000);
error:
	return result;
}

static int slr429_begin(unsigned char mode, unsigned char chips, unsigned char ch, unsigned char gid, unsigned char src)
{
	int result=0;
	
	lora_rx_packet_length = 0;
	lora_rx_packet_offset = 0;
	lora_rx_read_lock = false;;

	// set mode
	if(mode == 1) {
		slr429_port->print("@MO01\r\n");
		result = wait_msg_timeout(NULL,1000);
		result = wait_msg_timeout(NULL,1000);
		result = wait_msg_timeout("FSK CMD MODE",2000);
		if(result != 0) {
			result = -1;
			goto error;
		}
	} else if (mode == 3) {
		slr429_port->print("@MO03\r\n");
		result = wait_msg_timeout(NULL,1000);
		result = wait_msg_timeout(NULL,1000);
		result = wait_msg_timeout("LORA CMD MODE",2000);
		if(result != 0) {
			result = -1;
			goto error;
		}
	} else {
		goto error;
	}
	delay(100);
	// set chips
	if(chips<0x10)
	{
		slr429_port->print("@SF0");
	} else {
		slr429_port->print("@SF");
	}
	slr429_port->print_long(chips,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000)) != 0) {
		result = -2;
		goto error;
	}
	
	delay(100);
	// set ch
	if((ch < 7) || (ch > 46))
	{
		result = -3;
		goto error;
	} else if ((ch>=0x07) && (ch<0x10))
	{
		slr429_port->print("@CH0");
	} else {
		slr429_port->print("@CH");
	}
	slr429_port->print_long(ch,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000)) != 0) {
		result = -2;
		goto error;
	}
	
	delay(100);
	// set group id
	if (gid<0x10)
	{
		slr429_port->print("@GI0");
	} else {
		slr429_port->print("@GI");
	}
	slr429_port->print_long(gid,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000)) != 0) {
		result = -3;
		goto error;
	}
	
	delay(100);
	// set src
	if (src<0x10)
	{
		slr429_port->print("@EI0");
	} else {
		slr429_port->print("@EI");
	}
	slr429_port->print_long(src,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000)) != 0) {
		result = -3;
		goto error;
	}
	
error:
	return result;
}

static bool slr429_rx_update()
{
	int data;
	if(lora_rx_read_lock == false) {
		while(slr429_port->available()>0)
		{
			data = slr429_port->read();
#ifdef	DEBUG_LORA
				Serial.write_byte((unsigned char)data);
#endif
			lora_rx_buf[lora_rx_write_p] = (unsigned char)data;
			
			if(data == '\n') {	
				// データの解析
				strtok(lora_rx_buf,"=\r\n");
				if(strncmp(lora_rx_buf,"*DR",sizeof(lora_rx_buf)) == 0) {
					lora_rx_packet_length = lora_rx_write_p;
					lora_rx_packet_offset = 6;
					lora_rx_write_p=0;
					lora_rx_read_lock = true;
					break;
				}  else {
					lora_rx_write_p=0;
				}
			} else {
				lora_rx_write_p++;
			}
		}
	}
	return lora_rx_read_lock;
}

static short slr429_readData(unsigned char *data, short maxsize)
{
	int result=0;
	if(slr429_rx_update())
	{
		result = lora_rx_packet_length-lora_rx_packet_offset;
		memcpy(data,&lora_rx_buf[lora_rx_packet_offset],maxsize);
		lora_rx_packet_length = 0;
		lora_rx_packet_offset = 0;
		lora_rx_read_lock = false;
	}
	return result;
}

static int slr429_send(unsigned char dst, unsigned char *payload,int size)
{
	int result=0;
	
	if(size == 0) size = strlen(payload);
	
	delay(100);
	// set dst
	if (dst<0x10)
	{
		slr429_port->print("@DI0");
	} else {
		slr429_port->print("@DI");
	}
	slr429_port->print_long(dst,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000)) != 0) {
		result = -3;
		goto error;
	}
	
	delay(100);
	// set send
	if (size<0x10)
	{
		slr429_port->print("@DT0");
	} else {
		slr429_port->print("@DT");
	}
	slr429_port->print_long(size,HEX);
	slr429_port->write(payload,size);
	slr429_port->print("\r\n");
	
	// きちんと送信できたか確認を行う
	// エラーコードの生成

//	if((result = wait_msg_timeout(NULL,1000)) != 0) {
//		result = -3;
//		goto error;
//	}
	
error:
	return result;
}

const t_SLR429 slr429 = {
	slr429_init,
	slr429_begin,
	slr429_send,
	slr429_readData,
};

