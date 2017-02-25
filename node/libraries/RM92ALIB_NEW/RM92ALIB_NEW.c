
/* FILE NAME: rm92alib.c
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
#include "rm92alib_new.h"
#include "stdlib.h"

static unsigned char lora_rx_buf[256];
static int lora_rx_write_p;
static int lora_rx_packet_length;
static int lora_rx_packet_offset;
static bool lora_rx_read_lock;
static char rm92a_tx_buf[16];

static t_RM92A_CONFIG rm92a_config;
static HardwareSerial* rm92a_port;


#define RESET_MSG				"Transmit RF Mode FSK or LORA?  [1:LORA  2:FSK]"
#define RESET_DELIMITER			"=\r\n"
#define SETTING_MSG				NULL
#define SETTING_DELIMITER		">\r\n"
#define CH_MSG					"CH-NO[24 to 61]"
#define CH_DELIMITER			"=\r\n"
#define PANID_MSG1				"PAN Address Enable[0:Not Use  1:Use]"
#define	PANID_DELIMITER1		"=\r\n"
#define PANID_MSG2				"PAN-Address NO[1 to 65534]"
#define	PANID_DELIMITER2		"=\r\n"
#define SRC_MSG					"SRC-Address NO[0 to 65534]"
#define	SRC_DELIMITER			"=\r\n"
#define DST_MSG					"LAST-Address NO[0 to 65535]"
#define DST_DELIMITER			"=\r\n"

#define MODE_MSG				"UNIT MODE[0:Parent  1:Child]"
#define MODE_DELIMITER			"=\r\n"

#define ACK_MSG1				"Ack Request Set[0:Not Use  1:Use]"
#define	ACK_DELIMITER1			"=\r\n"
#define ACK_MSG2				"Ack Timeout Set(sec) [1 to 5]"
#define	ACK_DELIMITER2			"=\r\n"
#define ACK_MSG3				"NoAck Retry Counter [0:Not Retry  1-5:Retry Counter]"
#define	ACK_DELIMITER3			"=\r\n"

#define START_MSG				"configration End. ---> System Start."
#define	START_DELIMITER			"\r\n"

#define LOAD_MSG				"EEPROM Data Read Finished."
#define LOAD_DELIMITER			"\r\n"

#define OUTPUT_MSG				"Recv Packet Output Set[1:RSSI Output Set  2:Transfer(Source) Address Output Set]"
#define OUTPUT_DELIMITER		"=\r\n"

#define RSSI_OUTPUT_MSG				"RSSI Output Set[0:Not Output  1:Output]"
#define RSSI_OUTPUT_DELIMITER			"=\r\n"

#define SRC_OUTPUT_MSG			"Transfer Address(Source) Output Set[0:Not Output  1:Output]"
#define SRC_OUTPUT_DELIMITER	"=\r\n"

#define CCA_MSG1				"Carrier Sense Enable[0:Not Use  1:Use]"
#define CCA_DELIMITER1			"=\r\n"

#define CCA_MSG2				"Carrier Sense Retry Counter [0:Not Retry  1-9:Retry Count]"
#define CCA_DELIMITER2			"=\r\n"
#define RF_MSG1				"RF Settings[1:TX-Power Set  2:Bandwidth Set  3:Factor(SF) Set]"
#define RF_MSG2				"RF Settings[1:TX-Power Set  2:RF Transmit BitRate Set]"
#define RF_TXPWR_MSG		"TX-Power Set[0:20mW[+13dBm] 1:4mW[+6dBm] 2:1mW[+0dBm]"
#define RF_BW_MSG			"Bandwidth Set[0:125kHz  1:250kHz  2:500kHz]"
#define RF_BW_MSG2			"RF Transmit BitRate Set(bps) [5000 to 300000]"
#define RF_SF_MSG			"Factor(SF) Set[0:SF6 1:SF7 2:SF8 3:SF9 4:SF10 5:SF11 6:SF12]"
#define RF_DELIMITER		"=\r\n"


#define TX_MSG			"[OK]"
#define TX_DELIMITER	" \r\n"

#define SUCCEEDED_MSG			"Configration Succeeded."
#define SUCCEEDED_DELIMITER		"\r\n"

struct s_rm92a_settings rm92a_settings = {
	0,		//	unsigned char debug;				// 0: normal   1: debug
	2,		//	unsigned char routing_mode;			// 0: Fixation 1: AutoRouting 2:NonRouting
	1,		//	unsigned char unit_mode;			// 0: pararent 1: child
	1,		// unsigned char dt_mode;				// Data Transfer Mode[0:Discharge  1:Frame  2:TimerSend  3:SleepTimerSend(Non Routing Only)]
	{		//	struct {
		1,	//		bool enb;
		1,	//		unsigned char timeout;
		5	//		unsigned char retry;
	},		//	} ack;
	{		//	struct {
		1,	//		bool rssi;
		1,	//		bool src;
	},		//	} output;
	{		//	struct {
		1,	//		bool enable;			// Carrier Sense Enable[0:Not Use  1:Use]
		5	//		unsigned char retry;	// Carrier Sense Retry Counter [0:Not Retry  1-9:Retry Count]
	},		//	} cca;
	{		//	struct {		for RF setting
		{	//	struct 			for LoRa		// RF Settings[1:TX-Power Set  2:Bandwidth Set  3:Factor(SF) Set]
			0,		//		unsigned char txpwr;	// 1: TX-Power Set[0:20mW[+13dBm] 1:4mW[+6dBm] 2:1mW[+0dBm]
			0,		//		unsigned char bw;		// 2: Bandwidth Set[0:125kHz  1:250kHz  2:500kHz]
			4		//		unsigned char sf;		// 3: Factor(SF) Set[0:SF6 1:SF7 2:SF8 3:SF9 4:SF10 5:SF11 6:SF12]
		},
		{	//	struct	for FSK			RF Settings[1:TX-Power Set  2:RF Transmit BitRate Set]=
			0,		//		unsigned char tx_pwr;	// 1: TX-Power Set[0:20mW[+13dBm] 1:4mW[+6dBm] 2:1mW[+0dBm]
			50000	//		unsigned long bw;		// 2: RF Transmit BitRate Set(bps) [5000 to 300000]
		}
	},		//	} rf;
	1		//	unsigned char rf_mode;				// 1: Lora     2: FSK
};

static int wait_msg_timeout(char* msg, char* delimiter, unsigned long limit_time)
{
	volatile unsigned long current_time;
	unsigned long start_time;
	char *i_delimiter;
	bool delimiter_flag;
	int delimiter_len;
	int data;
	int result = -1;
	int i;
	lora_rx_write_p = 0;
	start_time = millis();
	
	if(delimiter) i_delimiter = delimiter;
	else i_delimiter = "\r\n";
	
	delimiter_len = strlen(i_delimiter);
	
	do
	{
		if(rm92a_port->available()>0)
		{
			data = rm92a_port->read();
			lora_rx_buf[lora_rx_write_p] = (unsigned char)data;
			lora_rx_write_p++;
			if(rm92a_settings.debug) Serial.write_byte((unsigned char)data);
			delimiter_flag = false;
			for(i=0;i<delimiter_len;i++) {
				if(data == delimiter[i]) {
					delimiter_flag=true;
					break;
				}
			}
			
			if(delimiter_flag == true)
			{
				strtok(lora_rx_buf,i_delimiter);
				if(msg != NULL) {
					if(strncmp(lora_rx_buf,msg,sizeof(lora_rx_buf)) == 0) {
						result = 0;
						lora_rx_write_p = 0;
						break;
					}
				}
				lora_rx_write_p = 0;
			}
		}
		current_time = millis();
	} while(current_time < (start_time+limit_time));
	
	return result;
}

static int rm92a_reset(void)
{
	int result = 0;

	digitalWrite(rm92a_config.rstb,HIGH);
	delay(100);
	digitalWrite(rm92a_config.rstb,LOW);
	delay(100);
	digitalWrite(rm92a_config.rstb,HIGH);
	
	result = wait_msg_timeout(RESET_MSG,RESET_DELIMITER,3000L);
	
	return result;
}

static int rm92a_init(HardwareSerial* port,t_RM92A_CONFIG *config)
{
	int result = 0;
	
	if(port) rm92a_port = port;
	else {
		result = -1;
		goto error;
	}
	
	rm92a_port->begin(115200);
	
	memcpy(&rm92a_config,config,sizeof(t_RM92A_CONFIG));
	
	digitalWrite(rm92a_config.boot0,LOW);	
	digitalWrite(rm92a_config.rstb,LOW);	
	pinMode(rm92a_config.boot0,OUTPUT);
	pinMode(rm92a_config.rstb,OUTPUT);
	
error:
	return result;
}

static int rm92a_set_rfmode(unsigned char mode) {
	int result=0;
	if((mode == LORA920) || (mode == FSK920)) {
		rm92a_port->println_long((long)mode,DEC);
	} else {
		result = -2;
		goto error;
	}
	
	wait_msg_timeout(NULL,SETTING_DELIMITER,2000L);
error:
	return result;
}

static int rm92a_set_ch(unsigned char ch) {
	int result = 0;
	
	rm92a_port->print("a");
	if((result = wait_msg_timeout(CH_MSG, CH_DELIMITER, 500L)) != 0) {
		result = -4;
		goto error;
	}
	rm92a_port->print_long(ch,DEC);
	delay(10);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -5;
		goto error;
	}

	wait_msg_timeout(NULL, SETTING_DELIMITER, 500L);

error:
	return result;
}

static int rm92a_set_panid(unsigned short panid) {
	int result = 0;
	int i;
	
	rm92a_port->print("b");
	if((result = wait_msg_timeout(PANID_MSG1,PANID_DELIMITER1 , 500L)) != 0) {
		result = -6;
		goto error;
	}
	
	rm92a_port->print("1");
	delay(5);
	rm92a_port->print("\n");
	
	if((result = wait_msg_timeout(PANID_MSG2,PANID_DELIMITER2 , 500L)) != 0) {
		result = -7;
		goto error;
	}
	wait_msg_timeout(NULL," ", 500L);
	
	Print.init(rm92a_tx_buf,sizeof(rm92a_tx_buf));
	Print.l(panid,DEC);
	for(i=0;i<Print.len();i++)
	{
		if(rm92a_tx_buf[i] != NULL) {
			rm92a_port->write_byte(rm92a_tx_buf[i]);
		} else {
			break;
		}
		delay(5);
	}
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -8;
		goto error;
	}
	wait_msg_timeout(NULL, SETTING_DELIMITER, 500L);
error:
	return result;
}

static int rm92a_set_src(unsigned short src) {
	int result = 0;
	int i;
	
	rm92a_port->print("c");
	if((result = wait_msg_timeout(SRC_MSG,SRC_DELIMITER , 500L)) != 0) {
		result = -9;
		goto error;
	}
	
	Print.init(rm92a_tx_buf,sizeof(rm92a_tx_buf));
	Print.l(src,DEC);
	for(i=0;i<Print.len();i++)
	{
		if(rm92a_tx_buf[i] != NULL) {
			rm92a_port->write_byte(rm92a_tx_buf[i]);
		} else {
			break;
		}
		delay(5);
	}
	rm92a_port->print("\n");
	wait_msg_timeout(NULL, SETTING_DELIMITER, 500L);
	
error:
	return result;
}

static int rm92a_set_dst(unsigned short dst) {
	int result = 0;
	int i;
	
	rm92a_port->print("d");
	if((result = wait_msg_timeout(DST_MSG,DST_DELIMITER , 500L)) != 0) {
		result = -9;
		goto error;
	}
	
	Print.init(rm92a_tx_buf,sizeof(rm92a_tx_buf));
	Print.l(dst,DEC);
	for(i=0;i<Print.len();i++)
	{
		if(rm92a_tx_buf[i] != NULL) {
			rm92a_port->write_byte(rm92a_tx_buf[i]);
		} else {
			break;
		}
		delay(5);
	}
	rm92a_port->print("\n");
	wait_msg_timeout(NULL, SETTING_DELIMITER, 500L);
	
error:
	return result;
}

static int rm92a_set_unit_mode() {
	int result = 0;
	
	rm92a_port->print("e");
	if((result = wait_msg_timeout(MODE_MSG,MODE_DELIMITER , 500L)) != 0) {
		result = -11;
		goto error;
	}
	rm92a_port->print_long(rm92a_settings.unit_mode,DEC);
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG,SUCCEEDED_DELIMITER , 500L)) != 0) {
		result = -12;
		goto error;
	}
error:
	return result;
}

static int rm92a_set_dt_mode() {
	int result = 0;
	
	rm92a_port->print("i");
	wait_msg_timeout(NULL,NULL , 500L);
	
	rm92a_port->print_long(rm92a_settings.dt_mode,DEC);
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG,SUCCEEDED_DELIMITER , 500L)) != 0) {
		result = -12;
		goto error;
	}
	
error:
	return result;
}

static int rm92a_set_ack() {
	int result = 0;
	
	rm92a_port->print("h");
	if((result = wait_msg_timeout(ACK_MSG1,ACK_DELIMITER1 , 500L)) != 0) {
		result = -13;
		goto error;
	}
	if(rm92a_settings.ack.enb) {
		rm92a_port->print("1");
		delay(5);
		rm92a_port->print("\n");
		if((result = wait_msg_timeout(ACK_MSG2,ACK_DELIMITER2 , 500L)) != 0) {
			result = -14;
			goto error;
		}
		rm92a_port->print_long((long)rm92a_settings.ack.timeout,DEC);
		delay(5);
		rm92a_port->print("\n");
		
		if((result = wait_msg_timeout(ACK_MSG3,ACK_DELIMITER3 , 500L)) != 0) {
			result = -15;
			goto error;
		}
		rm92a_port->print_long((long)rm92a_settings.ack.retry,DEC);
		delay(5);
		rm92a_port->print("\n");
	} else {
		rm92a_port->print("0");
		delay(5);
		rm92a_port->print("\n");
	}
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -16;
		goto error;
	}
error:
	return result;
}

static int rm92a_set_output() {
	int result = 0;
	
	rm92a_port->print("l");			// RSSI
	if((result = wait_msg_timeout(OUTPUT_MSG,OUTPUT_DELIMITER , 500L)) != 0) {
		result = -17;
		goto error;
	}
	rm92a_port->print("1");
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(RSSI_OUTPUT_MSG, RSSI_OUTPUT_DELIMITER, 500L)) != 0) {
		result = -18;
		goto error;
	}
	rm92a_port->print_long(rm92a_settings.output.rssi,DEC);
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -19;
		goto error;
	}
	rm92a_port->print("l");			// SRC address output
	if((result = wait_msg_timeout(OUTPUT_MSG,OUTPUT_DELIMITER , 500L)) != 0) {
		result = -20;
		goto error;
	}
	rm92a_port->print("2");
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SRC_OUTPUT_MSG, SRC_OUTPUT_DELIMITER, 500L)) != 0) {
		result = -21;
		goto error;
	}
	rm92a_port->print_long(rm92a_settings.output.src,DEC);
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -22;
		goto error;
	}
	
error:
	return result;
}

static int rm92a_set_cca() {
	int result = 0;
	
	rm92a_port->print("m");			// CCA setting
	if((result = wait_msg_timeout(CCA_MSG1,CCA_DELIMITER1 , 500L)) != 0) {
		result = -23;
		goto error;
	}
	if(rm92a_settings.cca.enable) {
		rm92a_port->print("1");
		delay(5);
		rm92a_port->print("\n");
		if((result = wait_msg_timeout(CCA_MSG2, CCA_DELIMITER2, 500L)) != 0) {
			result = -24;
			goto error;
		}
		rm92a_port->print_long(rm92a_settings.cca.retry,DEC);
		delay(5);
		rm92a_port->print("\n");
	} else {
		rm92a_port->print("0");
		delay(5);
		rm92a_port->print("\n");
	}
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -25;
		goto error;
	}
	
error:
	return result;
}

static int rm92a_set_rf_lora() {
	int result;
	
	// TX PWR
	rm92a_port->print("g");
	if((result = wait_msg_timeout(RF_MSG1,RF_DELIMITER , 500L)) != 0) {
		result = -26;
		goto error;
	}
	rm92a_port->print("1");
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(RF_TXPWR_MSG, RF_DELIMITER, 500L)) != 0) {
		result = -27;
		goto error;
	}
	rm92a_port->print_long(rm92a_settings.rf.lora.txpwr,DEC);
	delay(5);
	rm92a_port->print("\n");
	
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -28;
		goto error;
	}
	// BW
	rm92a_port->print("g");
	if((result = wait_msg_timeout(RF_MSG1,RF_DELIMITER , 500L)) != 0) {
		result = -29;
		goto error;
	}
	rm92a_port->print("2");
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(RF_BW_MSG, RF_DELIMITER, 500L)) != 0) {
		result = -30;
		goto error;
	}
	rm92a_port->print_long(rm92a_settings.rf.lora.bw,DEC);
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -31;
		goto error;
	}
	
	// SF
	rm92a_port->print("g");
	if((result = wait_msg_timeout(RF_MSG1,RF_DELIMITER , 500L)) != 0) {
		result = -32;
		goto error;
	}
	rm92a_port->print("3");
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(RF_SF_MSG, RF_DELIMITER, 500L)) != 0) {
		result = -33;
		goto error;
	}
	rm92a_port->print_long(rm92a_settings.rf.lora.sf,DEC);
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -34;
		goto error;
	}
error:
	return result;
}

static int rm92a_set_rf_fsk() {
	int result;
	int i;
	long j,k;
	bool out_flag = false;
	const long digit[] = {100000,10000,1000,100,10,1};
	
	// TX PWR
	rm92a_port->print("g");
	if((result = wait_msg_timeout(RF_MSG2,RF_DELIMITER , 500L)) != 0) {
		result = -35;
		goto error;
	}
	rm92a_port->print("1");
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(RF_TXPWR_MSG, RF_DELIMITER, 500L)) != 0) {
		result = -36;
		goto error;
	}
	rm92a_port->print_long(rm92a_settings.rf.fsk.txpwr,DEC);
	delay(5);
	rm92a_port->print("\n");
	
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -37;
		goto error;
	}
	// BW
	rm92a_port->print("g");
	if((result = wait_msg_timeout(RF_MSG2,RF_DELIMITER , 500L)) != 0) {
		result = -38;
		goto error;
	}
	rm92a_port->print("2");
	delay(5);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(RF_BW_MSG2, RF_DELIMITER, 500L)) != 0) {
		result = -39;
		goto error;
	}
	j = rm92a_settings.rf.fsk.bw;
	for(i=0;i<6;i++) {
		k = j/digit[i];
		if((out_flag != false) || (k != 0)) 
		{
			out_flag = true;
			rm92a_port->write_byte('0'+k);
			delay(1);
			j = j - (k*digit[i]);
		}
	}
	delay(1);
	rm92a_port->print("\n");
	if((result = wait_msg_timeout(SUCCEEDED_MSG, SUCCEEDED_DELIMITER, 500L)) != 0) {
		result = -40;
		goto error;
	}
	
error:
	return result;
}

static int rm92a_set_rf() {
	int result = 0;
	
	if(rm92a_settings.rf_mode == 1) {
		result = rm92a_set_rf_lora();
	} else {
		result = rm92a_set_rf_fsk();
	}
	
error:
	return result;
}

static int rm92a_begin(unsigned char mode, unsigned char ch, unsigned short panid, unsigned short src)
{
	int result=0;
	
	lora_rx_packet_length = 0;
	lora_rx_packet_offset = 0;
	lora_rx_read_lock = false;;
	
	// reset
	if((result = rm92a_reset())!=0) {
		result = -1;
		goto error;
	}
	// set rf_mode
	rm92a_settings.rf_mode = mode;
	if((result = rm92a_set_rfmode(mode)) != 0) goto error;

	
	// load
	/*
	if(load) {
		rm92a_port->print("y");
		wait_msg_timeout(NULL, SETTING_DELIMITER, 500L);
		return 0;
	}
	*/
	
	// set channel
	if((result = rm92a_set_ch(ch)) != 0) goto error;
	
	// set panid
	if((result = rm92a_set_panid(panid)) != 0) goto error;

	// set src
	if((result = rm92a_set_src(src)) != 0) goto error;

	// set dst
	if((result = rm92a_set_dst(1)) != 0) goto error;
	
	// unit mode
	if((result = rm92a_set_unit_mode()) != 0) goto error;
	
	// data transfer mode
	if((result = rm92a_set_dt_mode()) != 0) goto error;
	
	// ack config
	if((result = rm92a_set_ack() ) != 0) goto error;

	// output setting
	if((result = rm92a_set_output() ) != 0) goto error;
	
	// CCA setting
	if((result = rm92a_set_cca() ) != 0) goto error;
	
	// RF setting
	if((result = rm92a_set_rf()) != 0) goto error;
	
	// check parameters
	rm92a_port->print("?");
	wait_msg_timeout(NULL,SUCCEEDED_DELIMITER, 2000L);

	// start!!
	rm92a_port->print("s");
	if((result = wait_msg_timeout(START_MSG,START_DELIMITER , 500L)) != 0) {
		result = -42;
		goto error;
	}

error:
	lora_rx_write_p=0;
	return result;
}

static bool rm92a_rx_update()
{
	int data;
	if(lora_rx_read_lock == false) {
		while(rm92a_port->available()>0)
		{
			data = rm92a_port->read();
			
			if(rm92a_settings.debug) Serial.write_byte((unsigned char)data);
			
			lora_rx_buf[lora_rx_write_p] = (unsigned char)data;
			
			if(data == '\n') {	
				lora_rx_packet_length = lora_rx_write_p;
				lora_rx_write_p=0;
				lora_rx_read_lock = true;
				break;
			} else {
				lora_rx_write_p++;
			}
		}
	}
	return lora_rx_read_lock;
}

static short rm92a_readData(uint16_t *src,int16_t *rssi,uint8_t *payload, short maxsize)
{
	int result=0;
	bool lock;
	char *nazo;
	char *dstt;
	char *rssit;
	char *payloadt;
	int length;
	
	lock = rm92a_rx_update();
	if(lock)
	{
		nazo = strtok(lora_rx_buf,",\r\n");
		*src = (uint16_t)strtol(strtok(NULL,",\r\n"),NULL,0);
		*rssi = (int16_t)strtol(strtok(NULL,",\r\n"),NULL,0);
		payloadt = strtok(NULL,",\r\n");
		length = strlen(payloadt);
		if(rm92a_settings.debug) {
			Serial.print(dstt);
			Serial.print(",");
			Serial.print(rssit);
			Serial.print(",");
			Serial.print_long(length,DEC);
			Serial.print(",");
			Serial.print_long(payloadt[0],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[1],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[2],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[3],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[4],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[5],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[6],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[7],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[8],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[9],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[10],HEX);
			Serial.print(",");
			Serial.print_long(payloadt[11],HEX);
			Serial.println("");
		}

		result = (maxsize < length) ? maxsize : length;
		memcpy(payload,payloadt,result);
		
		lora_rx_read_lock=false;
	}
	return result;
}

unsigned char txbuf[128];
static int rm92a_send(unsigned short dst,unsigned char *payload,int size)
{
	int i;
	int result;
	if(size == 0) size = strlen(payload);
	txbuf[0] = '@';
	txbuf[1] = '@';
	txbuf[2] = size;
	txbuf[3] = (unsigned char)((dst >> 8) & 0xFF);
	txbuf[4] = (unsigned char)((dst >> 0) & 0xFF);
	strcpy(&txbuf[5],payload,size);
	txbuf[size+5] = 0xAA;
	result = size + 6;
	if(rm92a_settings.debug) {
		for(i=0;i<result;i++) {
			Serial.print_long(txbuf[i],HEX);
			Serial.print(",");
		}
		Serial.println("");
	}
	
	rm92a_port->write(txbuf,result);

	if((result = wait_msg_timeout(TX_MSG,TX_DELIMITER , 5000L)) != 0) {
		result = -1;
	}
	wait_msg_timeout(NULL,SUCCEEDED_DELIMITER, 500L);
error:
	lora_rx_write_p=0;
	return result;
}

void rm92a_setMode(struct s_rm92a_settings *settings) {
	memcpy(&rm92a_settings,settings,sizeof(rm92a_settings));
}
void rm92a_getMode(struct s_rm92a_settings *settings) {
	memcpy(settings,&rm92a_settings,sizeof(rm92a_settings));
}
const t_RM92A rm92a = {
	rm92a_init,
	rm92a_begin,
	rm92a_send,
	rm92a_readData,
	rm92a_setMode,
	rm92a_getMode
};

