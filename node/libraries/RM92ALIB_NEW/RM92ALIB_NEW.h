
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

#define LORA920	1
#define FSK920	2

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
		struct {
			unsigned char txpwr;	// 1:TX-Power Set[0:20mW[+13dBm] 1:4mW[+6dBm] 2:1mW[+0dBm]
			unsigned char bw;		// 2: Bandwidth Set[0:125kHz  1:250kHz  2:500kHz]
			unsigned char sf;		// 3: Factor(SF) Set[0:SF6 1:SF7 2:SF8 3:SF9 4:SF10 5:SF11 6:SF12]
		} lora;
		struct {
			unsigned char txpwr;	// TX-Power Set[0:20mW[+13dBm] 1:4mW[+6dBm] 2:1mW[+0dBm]
			unsigned long bw;		// RF Transmit BitRate Set(bps) [5000 to 300000]
		} fsk;
	} rf;
	unsigned char rf_mode;				// 1: Lora     2: FSK
};

typedef struct {
	int (*init)(HardwareSerial* port,t_RM92A_CONFIG *config);
	int (*begin)(unsigned char mode, unsigned char ch, unsigned short panid, unsigned short src);
	int (*send)(unsigned short dst,unsigned char *payload,int size);
	short (*readData)(uint16_t *dst,int16_t *rssi,uint8_t *data, short maxsize);
	void (*setMode)(struct s_rm92a_settings *settings);
	void (*getMode)(struct s_rm92a_settings *settings);
} t_RM92A;

extern const t_RM92A rm92a;

#endif		//_RM92A_H_

