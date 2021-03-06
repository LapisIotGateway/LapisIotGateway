
/* FILE NAME: slr429.h
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


#ifndef		_RM92A_H_
#define		_RM92A_H_

#include "lazurite.h"

enum RM92A_MODE {
	LORA_MODE=1,
	FSK_MODE=2
};

typedef struct {
		char boot0;
		char rstb;
}t_RM92A_CONFIG;

struct s_rm92a_settings  {
	unsigned char debug;				// 0: normal   1: debug
	unsigned char routing_mode;			// 0: Fixation 1: AutoRouting 2:NonRouting
	unsigned char unit_mode;			// 0: pararent 1: child
	unsigned char dt_mode;				// Data Transfer Mode[0:Discharge  1:Frame  2:TimerSend  3:SleepTimerSend(Non Routing Only)]
	struct {
		bool enb;
		unsigned char timeout;
		unsigned char retry;
	} ack;
	struct {
		bool rssi;
		bool src;
	} output;
	struct {
		bool enable;
		unsigned char retry;
	} cca;
	struct {
		unsigned char txpwr;	// 1:TX-Power Set
		unsigned char tts;		// 2:Transmit Time-Total Set
		unsigned char dts;		// 3:Transmit Down-Time Set
		unsigned char bw;		// 4:Bandwidth Set
		unsigned char sf;		// 5:Factor(SF) Set
	} rf;
	unsigned char rf_mode;				// 1: Lora     2: FSK
};

typedef struct {
	int (*init)(HardwareSerial* port,t_RM92A_CONFIG *config);
	int (*begin)(unsigned char mode, unsigned char ch, unsigned short panid, unsigned short src,unsigned short dst,bool load);
	int (*send)(unsigned short dst,unsigned char *payload,int size);
	short (*readData)(uint16_t *dst,int16_t *rssi,uint8_t *data, short maxsize);
	void (*setMode)(struct s_rm92a_settings *settings);
	void (*getMode)(struct s_rm92a_settings *settings);
} t_RM92A;

extern const t_RM92A rm92a;

#endif		//_RM92A_H_

