
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


#ifndef		_SLR429_H_
#define		_SLR429_H_

#include "lazurite.h"

enum LORA_MODE {
	FSK_CMD = 1,
	LORA_CMD = 3
};
typedef struct {
		char rts;
		char cts;
		char ini;
		char rstb;
}t_SLR429_CONFIG;

typedef struct {
	int (*init)(HardwareSerial* port,t_SLR429_CONFIG *config);
	int (*begin)(unsigned char mode, unsigned char chips, unsigned char ch, unsigned char gid, unsigned char src);
	int (*send)(unsigned char dst, unsigned char *payload,int size);
	short (*readData)(unsigned char *data, short max_size);
} t_SLR429;

extern const t_SLR429 slr429;

#endif		//_SLR429_H_

