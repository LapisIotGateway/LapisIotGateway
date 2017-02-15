#define LIB_DEBUG
#define BREAK_MODE

#include "rm92a_rx_ide.h"		// Additional Header

#define	LORA920_BOOT0		4
#define	LORA920_RST			5

#define LORA920_CH			60
#define LORA920_PANID		0xFFFC
#define LORA920_SRC			0x01
#define LORA920_DST			0x02
#define LORA920_MODE		24

#define ORANGE				25
#define BLUE				26

const t_RM92A_CONFIG rm92a_config =
{
	LORA920_BOOT0,
	LORA920_RST
};

struct s_rm92a_settings param;
void setup() {
	// put your setup code here, to run once:
	int result;

	Serial.begin(115200);

//	rm92a.getMode(&param);
//	param.debug = true;
//	rm92a.setMode(&param);
	
	if(( result = rm92a.init(&Serial1,&rm92a_config)) != 0)
	{
		Serial.print("error!! = ");
		Serial.println_long(result,DEC);
	}
//	BREAK("rm92a.begin");
	if(( result = rm92a.begin(FSK_MODE,LORA920_CH,LORA920_PANID,LORA920_SRC,LORA920_DST,false)) != 0)
	{
		Serial.print("error!! = ");
		Serial.println_long(result,DEC);
	}
	
	pinMode(ORANGE,OUTPUT);
	pinMode(BLUE,OUTPUT);
	
}
unsigned char payload[100];
void loop() {
	unsigned short src;
	short rssi;
	int result;
	result = rm92a.readData(&src,&rssi,payload,sizeof(payload));
	if(result > 0) {
		payload[result] = 0;
		Serial.print_long(src,HEX);
		Serial.print(",");
		Serial.print_long(rssi,DEC);
		Serial.print(",");
		Serial.println(payload);
	}
	return;
}
