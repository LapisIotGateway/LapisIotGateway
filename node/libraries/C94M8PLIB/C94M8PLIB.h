
/* FILE NAME: c94m8plib.h
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


#ifndef		_C94M8PLIB_
#define		_C94M8PLIB_

#include "lazurite.h"

struct s_c94m8p_settings {
	bool debug;
};

typedef struct {
	void (*init)(HardwareSerial* port,long baud);
	void (*setRTK)(unsigned char *data, int legth);
	bool (*getGNRMC)(char *gnrmc,int maxsize,unsigned long timeout);
	void (*getMode)(struct s_c94m8p_settings *mode);
	void (*setMode)(struct s_c94m8p_settings *mode);
} t_C94M8P;

extern const t_C94M8P c94m8p;

#endif		//_RM92A_H_

