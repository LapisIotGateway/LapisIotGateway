
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

static unsigned char lora_rx_buf[256];
static int lora_rx_write_p;
static bool lora_rx_read_lock;

static unsigned char *rx_buf;
static int rx_size;

static t_SLR429_CONFIG slr429_config;
struct s_slr429_settings slr429_settings = {
	0,		// debug
	0		// chips
};
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
			
			if(slr429_settings.debug) Serial.write_byte((unsigned char)data);
			
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
	
	result = wait_msg_timeout("*RESET",3000L);
	result = wait_msg_timeout(NULL,3000L);
error:
	return result;
}

static int slr429_begin(unsigned char mode, unsigned char ch, unsigned char gid, unsigned char src)
{
	int result=0;
	
	lora_rx_read_lock = false;;
	
	delay(500);

	// set mode
	if(mode == 1) {
		slr429_port->print("@MO01\r\n");
		result = wait_msg_timeout(NULL,1000L);
		result = wait_msg_timeout(NULL,1000L);
		result = wait_msg_timeout("FSK CMD MODE",2000L);
		if(result != 0) {
			result = -1;
//			goto error;
		}
	} else if (mode == 3) {
		slr429_port->print("@MO03\r\n");
		result = wait_msg_timeout(NULL,1000L);
		result = wait_msg_timeout(NULL,1000L);
		result = wait_msg_timeout("LORA CMD MODE",2000L);
		if(result != 0) {
			result = -1;
//			goto error;
		}
	} else {
		goto error;
	}
	delay(100);
	// set chips
	if(slr429_settings.chips<0x10)
	{
		slr429_port->print("@SF0");
	} else {
		slr429_port->print("@SF");
	}
	slr429_port->print_long((long)slr429_settings.chips,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000L)) != 0) {
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
	slr429_port->print_long((long)ch,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000L)) != 0) {
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
	slr429_port->print_long((long)gid,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000L)) != 0) {
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
	slr429_port->print_long((long)src,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000L)) != 0) {
		result = -3;
		goto error;
	}
	
error:
	return result;
}

static bool slr429_rx_update()
{
	int data;
	char *cmd;
	char *payload;
	char payload_len[3];
	if(lora_rx_read_lock == false) {
		while(slr429_port->available()>0)
		{
			data = slr429_port->read();
			
			if(slr429_settings.debug) Serial.write_byte((unsigned char)data);

			lora_rx_buf[lora_rx_write_p] = (unsigned char)data;
			lora_rx_write_p++;
			if(data == '\n') {	
				// ƒf[ƒ^‚Ì‰ðÍ
				lora_rx_buf[lora_rx_write_p] = NULL;
				cmd = strtok(lora_rx_buf,"=\r\n");
				if(strncmp(lora_rx_buf,"*DR",sizeof(lora_rx_buf)) == 0) {
					payload = strtok(NULL,"\r\n");
					payload_len[0] = payload[0];
					payload_len[1] = payload[1];
					payload_len[2] = NULL;
					rx_size = strtol(payload_len,NULL,16);
					rx_buf = payload+2;
					lora_rx_write_p=0;
					lora_rx_read_lock = true;
					break;
				}  else {
					lora_rx_write_p=0;
				}
			} 
		}
	}
	return lora_rx_read_lock;
}

static int slr429_getRSSI() {
	volatile unsigned long current_time;
	unsigned long start_time;
	int data;
	int result = -1;
	char *st;

	slr429_port->print("@RS\r\n");
	lora_rx_write_p = 0;
	start_time = millis();

	do
	{
		if(slr429_port->available()>0)
		{
			data = slr429_port->read();
			lora_rx_buf[lora_rx_write_p] = (unsigned char)data;
			lora_rx_write_p++;
			
			if(slr429_settings.debug) Serial.write_byte((unsigned char)data);
			
			if(data == '\n')
			{
				st = strtok(lora_rx_buf,"=\r\n");
				if(strncmp(st,"RSSI",sizeof(lora_rx_buf))==0) {
					result = strtol(strtok(NULL,"=\r\n"),NULL,0);
				} else {
					result = 0x7fff;
				}
				break;
			}
		}
		current_time = millis();
	} while(current_time < (start_time+1000));
	return result;
}

static short slr429_readData(uint16_t *dst,int16_t *rssi,uint8_t *data, short maxsize)
{
	int result=0;
	if(slr429_rx_update())
	{
		result = ((maxsize > rx_size) ? rx_size : maxsize);
		memcpy(data,rx_buf,result);
		lora_rx_read_lock = false;
		
		if(slr429_settings.debug) {
			Serial.print("payload :: ");
			Serial.write(data,result);
			Serial.println("");
		}
		
		*rssi = slr429_getRSSI();
		if(slr429_settings.debug)  {
			Serial.print("rssi = ");
			Serial.println_long(*rssi,DEC);
		}
	}
	return result;
}

static char tx_result[16];
static int slr429_send(unsigned char dst, unsigned char *payload,int size)
{
	int result=0;
	int i,data;
	volatile unsigned long current_time;
	volatile unsigned long start_time;
	if(size == 0) size = strlen(payload);
	
	delay(100);
	// set dst
	if (dst<0x10)
	{
		slr429_port->print("@DI0");
	} else {
		slr429_port->print("@DI");
	}
	slr429_port->print_long((long)dst,HEX);
	slr429_port->print("\r\n");
	if((result = wait_msg_timeout(NULL,1000L)) != 0) {
		result = -3;
		goto error;
	}
	
	delay(100);
	// set send
	if (size<0x10)
	{
		slr429_port->print("@DT0");
		if(slr429_settings.debug) 	Serial.print("@DT0");
	} else {
		slr429_port->print("@DT");
		if(slr429_settings.debug) 	Serial.print("@DT");
	}
	slr429_port->print_long((long)size,HEX);
	if(slr429_settings.debug) 	Serial.print_long((long)size,HEX);
	slr429_port->write(payload,size);
	if(slr429_settings.debug) 	Serial.write(payload,size);
	slr429_port->print("\r\n");
	if(slr429_settings.debug) 	Serial.print("\r\n");
	
	start_time = millis();
	i = 0;
	while(1) {
		if(slr429_port->available() > 0) {
			data = slr429_port->read();
			tx_result[i] = (char)data;
			
			if(slr429_settings.debug) {
				Serial.write_byte(data);
			}
			i++;
			if(data == '\n') {
				tx_result[i]=NULL;
				if(strncmp(strtok(tx_result,"=\r\n"),"*DT",sizeof(tx_result)) == 0) {
					break;
				} else {
					result = -4;
					goto error;
				}
			}
		}
		if((start_time+5000) < millis()) {
			result = -5;
			goto error;
		}
	}
	i = 0;
	while(1) {
		if(slr429_port->available() > 0) {
			data = slr429_port->read();
			tx_result[i] = (char)data;
			i++;
			if(slr429_settings.debug) {
				Serial.write_byte(data);
			}
			if(data == '\n') {
				tx_result[i]=NULL;
				if(strncmp(strtok(tx_result,"\r\n"),"*IR=03",sizeof(tx_result)) == 0) {	// OK
					result = 0;
					break;
				} else {			// CCA error
					result = -6;
					break;
				}
			}
		}
		if((start_time+5000) < millis()) {
			result = -7;
			goto error;
		}
	}
	
error:
	return result;
}
void slr429_setMode(struct s_slr429_settings *settings) {
	memcpy(&slr429_settings,settings,sizeof(slr429_settings));
}
void slr429_getMode(struct s_slr429_settings *settings) {
	memcpy(settings,&slr429_settings,sizeof(slr429_settings));
}

const t_SLR429 slr429 = {
	slr429_init,
	slr429_begin,
	slr429_send,
	slr429_readData,
	slr429_setMode,
	slr429_getMode
};

