#include "sensor_node_ide.h"		// Additional Header

#define RF_MODE LORA429				// Chose from LORA429, FSK429, LORA920, FSK920
//#define RTK
#define DEBUG 1

#if RF_MODE == FSK920
	#define RF_MODULE		rm92a
	#define RM92A
#elif RF_MODE == LORA920
	#define RF_MODULE		rm92a
	#define RM92A
#elif RF_MODE == FSK429
	#define RF_MODULE		slr429
	#define SLR429
#elif RF_MODE == LORA429
	#define RF_MODULE		slr429
	#define SLR429
#else
	#error RF_MODE must be set
#endif

#ifdef	RM92A
// LORA 920 Initial Parameter
	#define	LORA920_BOOT0		4
	#define	LORA920_RST			5
	
	#define RF_CH				24
	#define RF_PANID			0x1234
	#define RF_SRC				0x2
	#define RF_DST				0x1
	
	static const t_RM92A_CONFIG config =
	{
		LORA920_BOOT0,
		LORA920_RST
	};
	static struct s_rm92a_settings param;

#endif

// LORA429 Initial Parameter
#ifdef SLR429
	#define	LORA429_RTS			2
	#define	LORA429_CTS			3
	#define	LORA429_INI			4
	#define	LORA429_RSTB		5
	
	#define RF_CH				27
	#define RF_PANID			0
	#define RF_SRC				2
	#define RF_DST				1
	
	static const t_SLR429_CONFIG config ={
		LORA429_RTS,	//		char rts;
		LORA429_CTS,	//		char cts;
		LORA429_INI,	//		char ini;
		LORA429_RSTB	//		char rstb;
	};
	
	static struct s_slr429_settings param;
#endif

// RTK parameters
#ifdef RTK
	struct s_rtk_param {
		bool success;
		unsigned char data[1024];
		unsigned char size;
		unsigned char id;
		int index;
		int length;
	} rtk = {
		false
	};

	static bool rx_rtk_packet_filter(char* payload)
	{
		char *flag;
		int packet_size;
		int packet_id;
		char* st;
		static unsigned char conv_data;
		int i;
	
		flag = strtok(payload,";\r\n");
	//	Serial.println(flag);
	
		packet_id = strtol(strtok(NULL,";\r\n"),NULL,10);
	//	Serial.println_long(packet_id,DEC);
		
		packet_size = strtol(strtok(NULL,";\r\n"),NULL,10);
	//	Serial.println_long(packet_size,DEC);
	
		st = strtok(NULL,";\r\n");
	
		if(strncmp(flag,"@F",5)!=0) {
			
			rtk.success = false;
			digitalWrite(ORANGE,HIGH);
			return false;
		}
	
		if(packet_id == 0) {
			digitalWrite(ORANGE,LOW);
			rtk.success = true;
			rtk.size = packet_size;
			rtk.id = packet_id;
			rtk.index = 0;
		} else {
			if((rtk.id != (packet_id - 1)) || (rtk.size != packet_size) ) {
				rtk.success = false;
				digitalWrite(ORANGE,HIGH);
				return false;		
			} else {
				rtk.id = packet_id;
			}
		}
		
		i=0;
		while(st[i] != NULL) {
			if((rtk.index & 0x01) == 0) {
				if((st[i] >= '0') && (st[i] <= '9')) {
					conv_data = st[i] - '0';
				} else if ((st[i] >= 'A') && (st[i] <= 'F')) {
					conv_data = st[i] - 'A' + 10;
				} else if  ((st[i] >= 'a') && (st[i] <= 'f')) {
					conv_data = st[i] - 'a' + 10;
				}
	//			gnss.data[gnss.index>>1] = conv_data;
			} else {
				conv_data <<= 4;
				if((st[i] >= '0') && (st[i] <= '9')) {
					conv_data += st[i] - '0';
				} else if ((st[i] >= 'A') && (st[i] <= 'F')) {
					conv_data += st[i] - 'A' + 10;
				} else if  ((st[i] >= 'a') && (st[i] <= 'f')) {
					conv_data += st[i] - 'a' + 10;
				}
				
				rtk.data[rtk.index>>1] = conv_data;
				
	//			if(gnss.data[gnss.index>>1]<16) Serial.print("0");
	//			Serial.print_long(gnss.data[gnss.index>>1],HEX);
			}
			rtk.index++;
			i++;
		}
	//	Serial.println("");
		if(rtk.id == (rtk.size - 1)) {
	//		Serial.println("packet ok!!");
			rtk.success = false;
			rtk.length = (rtk.index >> 1);
			digitalWrite(ORANGE,HIGH);
			return true;
		}
		return false;
	}
#endif


// COMMON PARAMETER
#define ORANGE				25
#define BLUE				26

unsigned char gnss_payload[100];
char rf_payload[256];


static bool rx_nortk_packet_filter(char* payload) {
	bool result = true;

	if(strncmp(payload,"@trigger",sizeof(payload)) == 0) {
		result = true;
	} else {
		result = false;
	}
	
	return result;
}

void setup() {
	// put your setup code here, to run once:
	int result;

	// set GPIO
	pinMode(ORANGE,OUTPUT);
	pinMode(BLUE,OUTPUT);

	// set GPS modle
#ifdef RTK
	c94m8p.init(&Serial,9600L);
#else
	c94m8p.init(&Serial,115200L);
#endif

	RF_MODULE.getMode(&param);
#ifdef DEBUG
	param.debug = true;
#endif
	RF_MODULE.setMode(&param);
	result = RF_MODULE.init(&Serial1,&config);

	if( result != 0 ) 	{
		while(1) {
			digitalWrite(BLUE,LOW);
			digitalWrite(ORANGE,LOW);
			delay(500);
			digitalWrite(BLUE,HIGH);
			digitalWrite(ORANGE,HIGH);
			delay(500);
		}
	}

	result = RF_MODULE.begin(RF_MODE,RF_CH,RF_PANID,RF_SRC);
	
	if(result != 0) {
		while(1) {
			digitalWrite(BLUE,LOW);
			digitalWrite(ORANGE,LOW);
			delay(1000);
			digitalWrite(BLUE,HIGH);
			digitalWrite(ORANGE,HIGH);
			delay(1000);
		}
	}
}

void loop() {
	unsigned short src;
	static bool isSendRtk = 0;
	static bool isSendGnss = 0;
	short rssi;
	int result;
	
	result = RF_MODULE.readData(&src,&rssi,rf_payload,sizeof(rf_payload));

	if(result > 0) {
		rf_payload[result] = 0;		
#ifdef RTK
		switch(rx_rtk_packet_filter(rf_payload)) {
		case true:
			isSendRtk = true;
			isSendGnss = true;
			break;
		default:
			isSendRtk = false;
			isSendGnss =true;
			break;
		}
#else
		switch(rx_nortk_packet_filter(rf_payload)) {
		case true:
			digitalWrite(BLUE,LOW);
			digitalWrite(ORANGE,LOW);
			isSendRtk = false;
			isSendGnss = true;
			break;
		default:
			isSendRtk = false;
			isSendGnss =false;
			break;
		}
#endif
	}

#ifdef	RTK
	if(isSendRtk) {
		c94m8p.setRTK(rtk.data,rtk.length);
		digitalWrite(BLUE,HIGH);
		isSendRtk = false;
	}
#endif
	if(isSendGnss) {
		if(c94m8p.getGNRMC(gnss_payload,sizeof(gnss_payload),5000)){
			digitalWrite(BLUE,HIGH);
			RF_MODULE.send(1,gnss_payload,NULL);
		} else {
			digitalWrite(ORANGE,HIGH);
			RF_MODULE.send(1,"GPS NOSIGNAL",NULL);
		}
		isSendGnss = false;
	}
	return;
}
