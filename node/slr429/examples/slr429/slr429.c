#include "slr429_ide.h"		// Additional Header

#define	LORA429_RTS		2
#define	LORA429_CTS		3
#define	LORA429_INI		4
#define	LORA429_RSTB	5

#define LORA429_CH		0x10
#define LORA429_MODE	FSK_CMD
#define LORA429_GID		0x01
#define LORA429_CHIPS	0x0
#define LORA429_SRC		2
#define LORA429_DST		1

#define ORANGE				25
#define BLUE				26

// LORA module is not connected and data for RM-92A is output to PC, please set to DEBUG_LORA option

char txbuf[128];

t_SLR429_CONFIG slr429_config ={
	LORA429_RTS,	//		char rts;
	LORA429_CTS,	//		char cts;
	LORA429_INI,	//		char ini;
	LORA429_RSTB	//		char rstb;
};

void setup(void) {
	// put your setup code here, to run once:
	int result;

	Serial.begin(115200);
	
	result = slr429.init(&Serial1,&slr429_config);
	if(result == 0) Serial.println("init success");
	else  Serial.println("init faulse");

	result = slr429.begin(LORA429_MODE,LORA429_CHIPS,LORA429_CH,LORA429_GID,LORA429_SRC);
	if(result == 0) Serial.println("begin success");
	else  Serial.println("begin faulse");
	
	result = slr429.send(0x00,"hello",sizeof("hello")-1);
//	if(result == 0) Serial.println("begin success");
//	else  Serial.println("begin faulse");

}
static unsigned char rx_buf[256];
void loop(void) {
	int result;
	// put your main code here, to run repeatedly:

	if((result = slr429.readData(rx_buf,sizeof(rx_buf))) > 0)
	{
		Serial.write(rx_buf,result);
		Serial.println("");
	} else {
		result = slr429.send(0x00,"no data",NULL);
	}
	delay(5000);
}
