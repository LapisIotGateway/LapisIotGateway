#define LIB_DEBUG
#define BREAK_MODE

#include "rm92a_tx_ide.h"		// Additional Header

#define	LORA920_BOOT0		4
#define	LORA920_RST			5

#define LORA920_CH			60
#define LORA920_PANID		0xFFFC
#define LORA920_SRC			0x04
#define LORA920_DST			0x01
#define LORA920_MODE		24

#define ORANGE				25
#define BLUE				26

const t_RM92A_CONFIG rm92a_config =
{
	LORA920_BOOT0,
	LORA920_RST
};

static struct s_rm92a_settings setting;

void setup() {
	// put your setup code here, to run once:
	int result;
	
	pinMode(ORANGE,OUTPUT);
	pinMode(BLUE,OUTPUT);

	Serial.begin(115200);

//	rm92a.getMode(&setting);
//	setting.debug = true;
//	rm92a.setMode(&setting);
	
	if(( result = rm92a.init(&Serial1,&rm92a_config)) != 0)
	{
//		Serial.print("error!! = ");
//		Serial.println_long(result,DEC);
	}
//	BREAK("rm92a.begin");
	if(( result = rm92a.begin(FSK_MODE,LORA920_CH,LORA920_PANID,LORA920_SRC,LORA920_DST,false)) != 0)
	{
		digitalWrite(26,HIGH);
		while(1);
	}
	
	
}

void loop() {
	unsigned long current_time;
	static int num=0;
	static unsigned long previous_time = 0x7FFF;
	int result;
	// put your main code here, to run repeatedly:
//	if(Serial.available()>0)
//	{
//		Serial1.write_byte((uint8_t)Serial.read());
//		digitalWrite(BLUE,LOW);
//	} else {
//		digitalWrite(BLUE,HIGH);
//	}
//	if(Serial1.available()>0)
//	{
//		Serial.write_byte((uint8_t)Serial1.read());
//		digitalWrite(ORANGE,LOW);
//	} else {
//		digitalWrite(ORANGE,HIGH);
//	}
	current_time = millis();
	if((current_time - previous_time) > 10000) {
		digitalWrite(BLUE,LOW);
		switch(num%3) {
			case 0:
				result = rm92a.send(LORA920_DST,"hello",NULL);
				break;
			case 1:
				result = rm92a.send(LORA920_DST,"hello world",NULL);
				break;
			default:
				result = rm92a.send(LORA920_DST,"hhhheeeelllloooo wwwwrrrroooolllldddd!!!!",NULL);
				break;
		}
		num++;
		if(result == 0 ) {
			Serial.println("OK");
		} else {
			Serial.println("NG");
		}
		digitalWrite(BLUE,HIGH);
		previous_time = current_time;
	}
}
