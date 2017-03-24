#include "RM92A_3_ide.h"		// Additional Header

#define RF_MODE FSK920				// Chose from LORA429, FSK429, LORA920, FSK920

#if RF_MODE == FSK920
#define RF_MODULE		rm92a
#define RM92A
//	#define DEBUG			1		// must be remove C94M8P
#elif RF_MODE == LORA920
#define RF_MODULE		rm92a
#define RM92A
#elif RF_MODE == FSK429
#define RF_MODULE		slr429
#define SLR429
//	#define DEBUG 1
#elif RF_MODE == LORA429
#define RF_MODULE		slr429
#define SLR429
//	#define DEBUG 1
#else
#error RF_MODE must be set
#endif

#define RF_SRC 3
#define GNRMC_OFFSET 2					// first 2byte of payload show src address. GNRMC data start after GNRMC_OFFSET[byte]

// COMMON PARAMETER
#define ORANGE				25
#define BLUE				26


#ifdef	RM92A
// LORA 920 Initial Parameter
#define	LORA920_BOOT0		4			// GPIO setting
#define	LORA920_RST			5			// GPIO setting

#define RF_CH				24			// RF frequency. Choose from 24-61
#define RF_PANID			0x1234		// PANID choose from 0x0000 - 0xFFFD
//	#define RF_SRC				0x2			// my address. choose from 2-3
#define RF_DST				0x1			// gateway address. it should be 1

static const t_RM92A_CONFIG config =
{
  LORA920_BOOT0,
  LORA920_RST
};
static struct s_rm92a_settings param;

#endif

// LORA429 Initial Parameter
#ifdef SLR429
#define	LORA429_RTS			2			// GPIO setting
#define	LORA429_CTS			3			// GPIO setting
#define	LORA429_INI			4			// GPIO setting
#define	LORA429_RSTB		5			// GPIO setting

#define RF_CH				27			// RF frequency  Choose from xxx-xxx
#define RF_PANID			0			// PANID choose from 0 - FF
//	#define RF_SRC				3			// my address. choose from 2 - 3.
#define RF_DST				1			// gateway address. it should be 1

static const t_SLR429_CONFIG config = {
  LORA429_RTS,	//		char rts;
  LORA429_CTS,	//		char cts;
  LORA429_INI,	//		char ini;
  LORA429_RSTB	//		char rstb;
};

static struct s_slr429_settings param;
#endif

static unsigned short rf_my_addr;


// BME280 related parameters
#define	BME280_I2C_ADDR			0x76		//I2C Address
#define	BME280_OSRS_T			1			//Temperature oversampling x 1
#define	BME280_OSRS_P			1			//Pressure oversampling x 1
#define	BME280_OSRS_H			1			//Humidity oversampling x 1
#define	BME280_MODE				1			//Normal mode
#define	BME280_T_SB				5			//Tstandby 1000ms
#define	BME280_FILTER			0			//Filter off
#define BME280_SPI_EN			0			//3-wire SPI Disable
char sensor_result[64];						// Sensor result
// RTK parameters
struct s_rtk_param {
  char success;
  unsigned char data[1024];
  unsigned char size;
  unsigned char id;
  int index;
  int length;
} rtk = {
  false
};

static char rx_rtk_packet_filter(char* payload)
{
  char *flag;
  int packet_size;
  int packet_id;
  char* st;
  static unsigned char conv_data;
  int i;

  flag = strtok(payload, ";\r\n");
  //	Serial.println(flag);

  packet_id = strtol(strtok(NULL, ";\r\n"), NULL, 10);
  //	Serial.println_long(packet_id,DEC);

  packet_size = strtol(strtok(NULL, ";\r\n"), NULL, 10);
  //	Serial.println_long(packet_size,DEC);

  st = strtok(NULL, ";\r\n");

  if (strncmp(flag, "@trigger", sizeof(16)) == 0)
  {
    digitalWrite(ORANGE, HIGH);
    rtk.success = false;
    return 2;
  } else
    if (strncmp(flag, "@F", 5) != 0) {

      rtk.success = false;
      digitalWrite(ORANGE, HIGH);
      return 0;
    }

  if (packet_id == 0) {
    digitalWrite(ORANGE, LOW);
    rtk.success = true;
    rtk.size = packet_size;
    rtk.id = packet_id;
    rtk.index = 0;
  } else {
    if ((rtk.id != (packet_id - 1)) || (rtk.size != packet_size) ) {
      rtk.success = false;
      digitalWrite(ORANGE, HIGH);
      return 0;
    } else {
      rtk.id = packet_id;
    }
  }

  i = 0;
  while (st[i] != NULL) {
    if ((rtk.index & 0x01) == 0) {
      if ((st[i] >= '0') && (st[i] <= '9')) {
        conv_data = st[i] - '0';
      } else
        if ((st[i] >= 'A') && (st[i] <= 'F')) {
          conv_data = st[i] - 'A' + 10;
        } else
          if  ((st[i] >= 'a') && (st[i] <= 'f')) {
            conv_data = st[i] - 'a' + 10;
          }
      //			gnss.data[gnss.index>>1] = conv_data;
    } else {
      conv_data <<= 4;
      if ((st[i] >= '0') && (st[i] <= '9')) {
        conv_data += st[i] - '0';
      } else
        if ((st[i] >= 'A') && (st[i] <= 'F')) {
          conv_data += st[i] - 'A' + 10;
        } else
          if  ((st[i] >= 'a') && (st[i] <= 'f')) {
            conv_data += st[i] - 'a' + 10;
          }

      rtk.data[rtk.index >> 1] = conv_data;

      //			if(gnss.data[gnss.index>>1]<16) Serial.print("0");
      //			Serial.print_long(gnss.data[gnss.index>>1],HEX);
    }
    rtk.index++;
    i++;
  }
  //	Serial.println("");
  if (rtk.id == (rtk.size - 1)) {
    //		Serial.println("packet ok!!");
    rtk.success = false;
    rtk.length = (rtk.index >> 1);
    digitalWrite(ORANGE, HIGH);
    return 1;
  }
  return 0;
}

unsigned char gnss_payload[256];
char rf_payload[256];

void setup() {
  // put your setup code here, to run once:
  int result;

  // set GPIO
  pinMode(ORANGE, OUTPUT);
  pinMode(BLUE, OUTPUT);

  pinMode(A0, INPUT_PULLUP);


  // Sensor Init
  Wire.begin();
  bme280.setMode(BME280_I2C_ADDR, BME280_OSRS_T, BME280_OSRS_P, BME280_OSRS_H, BME280_MODE, BME280_T_SB,
                 BME280_FILTER, BME280_SPI_EN);
  bme280.readTrim();

  // set GPS modle
  c94m8p.init(&Serial, 9600L);

  RF_MODULE.getMode(&param);
#ifdef DEBUG
  param.debug = true;
#endif
  RF_MODULE.setMode(&param);
  result = RF_MODULE.init(&Serial1, &config);

  if ( result != 0 ) 	{
    while (1) {
      digitalWrite(BLUE, LOW);
      digitalWrite(ORANGE, LOW);
      delay(500);
      digitalWrite(BLUE, HIGH);
      digitalWrite(ORANGE, HIGH);
      delay(500);
    }
  }

  //	rf_my_addr = (digitalRead(A0)==0) ? 2 : 3;
  rf_my_addr = RF_SRC;

  result = RF_MODULE.begin(RF_MODE, RF_CH, RF_PANID, rf_my_addr);

  if (result != 0) {
    while (1) {
      digitalWrite(BLUE, LOW);
      digitalWrite(ORANGE, LOW);
      delay(1000);
      digitalWrite(BLUE, HIGH);
      digitalWrite(ORANGE, HIGH);
      delay(1000);
    }
  }
}

void loop() {
  unsigned short src;
  static bool isSendRtk = 0;
  static bool isSendGnss = 0;
  double temp_act, press_act, hum_act;
  short rssi;
  int result;

  result = RF_MODULE.readData(&src, &rssi, rf_payload, sizeof(rf_payload));

  if (result > 0) {
    rf_payload[result] = 0;
    switch (rx_rtk_packet_filter(rf_payload)) {
      case 1:
        digitalWrite(BLUE, LOW);
        digitalWrite(ORANGE, LOW);
        isSendRtk = true;
        isSendGnss = true;
        break;
      case 2:
        digitalWrite(BLUE, HIGH);
        digitalWrite(ORANGE, LOW);
        isSendRtk = false;
        isSendGnss = true;
        break;
      default:
        digitalWrite(BLUE, HIGH);
        digitalWrite(ORANGE, LOW);
        isSendRtk = false;
        isSendGnss = false;
        break;
    }
  }

  if (isSendRtk) {
    c94m8p.setRTK(rtk.data, rtk.length);
    digitalWrite(BLUE, HIGH);
    isSendRtk = false;
  }
  if (isSendGnss) {
    if (RF_SRC) {
      gnss_payload[0] = '0' + rf_my_addr;
      gnss_payload[1] = ',';
      gnss_payload[2] = NULL;
    }
    if (c94m8p.getGNRMC(gnss_payload + GNRMC_OFFSET, sizeof(gnss_payload), 5000)) {
      strtok(gnss_payload, "\r\n");
      strcat(gnss_payload, ",");
      //			Serial.println("success to get GNRMC");
    }
    digitalWrite(ORANGE, HIGH);
    Print.init(sensor_result, sizeof(sensor_result));
    bme280.setMode(BME280_I2C_ADDR, BME280_OSRS_T, BME280_OSRS_P, BME280_OSRS_H,
                   BME280_MODE, BME280_T_SB, BME280_FILTER, BME280_SPI_EN);
    bme280.readData(&temp_act, &press_act, &hum_act);
    //		Serial.println("success to get BME280");RF_SRC
    Print.p("BME280,");
    Print.d(temp_act, 1);
    Print.p(",");
    Print.d(hum_act, 2);
    Print.p(",");
    Print.d(press_act, 2);
    strcat(gnss_payload, sensor_result);
    isSendGnss = false;
    result = RF_MODULE.send(1, gnss_payload, NULL);

#ifdef DEBUG
    if (result < 0) {
      Serial.print("SLR429 ERROR:: ");
      Serial.println_long(result, DEC);
    }
#endif
  }
  return;
}
