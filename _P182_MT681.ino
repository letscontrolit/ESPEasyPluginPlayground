//#######################################################################################################
//################ Plugin 98:ISKRA  MT681 smart meter									################### 
//#######################################################################################################

#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_182
#define PLUGIN_ID_182         182
#define PLUGIN_NAME_182       "MT681 [TESTING]"
#define PLUGIN_VALUENAME1_182 "PowerBuy"
#define PLUGIN_VALUENAME2_182 "PowerSell"
#define PLUGIN_VALUENAME3_182 "CurrentPower"
#include "Wire.h"

//==============================================
// MT681 LIBRARY - MT681.h
// =============================================
# ifndef MT681_H
# define MT681_H
#define SML_MSG_BUFFER_SIZE 500

typedef enum {
	WAITFORBEGIN,
	WAITFOREND,
	GETCHECKSUM,
	CHECKCRC
} ReceiveState;


class MT681
{
public:
	MT681();
	void get();
	unsigned int readSML();
	double currentPower; // aktuelle Wirkleistung. negativ=Einspeisung
	double SellCounterTotal; // EinspeisezÃ¤hler 281
	double BuyCounterTotal; // BezugszÃ¤hler 181   
	void emptyBuffers();
private:
	unsigned char FrameStart[8] = { 0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01 };
	unsigned char FrameEnd[5] = { 0x1b, 0x1b, 0x1b, 0x1b, 0x1a };
	unsigned char buffer[SML_MSG_BUFFER_SIZE];
	unsigned int bufferPtr;
	ReceiveState RState;
	unsigned int RStatePtr;
	double  getSMLValue(unsigned int pos);
	unsigned int  crcReflectw(uint16_t data);
	unsigned char crcReflectb(uint8_t data);
	unsigned int crcX25(uint8_t* data, uint16_t data_len);
	unsigned int  checkCRC(unsigned int frameEnd);

	int findValue(int pos);


};

#endif

//==============================================
// MT681 LIBRARY - MT681.cpp
// =============================================
MT681::MT681()
{
	emptyBuffers();
	currentPower = NAN; // aktuelle Wirkleistung. negativ=Einspeisung
	SellCounterTotal = NAN; // EinspeisezÃ¤hler 281
	BuyCounterTotal = NAN; // BezugszÃ¤hler 181
	// Serial.begin(9600); // done in the framework. Remember to adjust the speed in the advanced settings section
}




double MT681::getSMLValue(unsigned int pos) {
	yield();
	while (pos < pos + 32) {
		if (((buffer[pos] == 0x59) && (buffer[pos - 1] == 0xff)) || ((buffer[pos] == 0x55) && (buffer[pos - 1] == 0x00)))
			break;
		else
		{
			pos++;
			if (pos > SML_MSG_BUFFER_SIZE) return(0);
		}
	}
	if (pos > SML_MSG_BUFFER_SIZE - 7) return(0);
	if ((buffer[pos] == 0x59) || (buffer[pos] == 0x55))
		pos++;
	else
		return(0);


	if (buffer[pos - 1] == 0x59)
	{
		unsigned long tmp = 0;
		tmp = buffer[pos + 7];
		tmp += buffer[pos + 6] << 8;
		tmp += buffer[pos + 5] << 16;
		tmp += buffer[pos + 4] << 24;
		tmp += buffer[pos + 3] << 32;
		tmp += buffer[pos + 2] << 48;
		tmp += buffer[pos + 1] << 56;
		tmp += buffer[pos + 0] << 64;
		return((double)tmp / 10000);
	}
	else
	{
		int32_t tmp = 0;
		char* a = (char*)&tmp;
		a[3] = buffer[pos + 0];
		a[2] = buffer[pos + 1];
		a[1] = buffer[pos + 2];
		a[0] = buffer[pos + 3];
		return((double)tmp);
	}
}

int MT681::findValue(int pos) {
	// Serial.print ("findvalues "); Serial.println(pos);
	while (pos < SML_MSG_BUFFER_SIZE) {
		if ((buffer[pos] == 0x77) && (buffer[pos + 1] == 0x07))

		{
			pos = pos + 3;
			break;
		}
		else
			pos++;
	}
	if (pos >= SML_MSG_BUFFER_SIZE) return (SML_MSG_BUFFER_SIZE);
	if ((buffer[pos + 1] == 0x01) && (buffer[pos + 2] == 0x08) && (buffer[pos + 3] == 0x01))
	{
		BuyCounterTotal = getSMLValue(pos);
	}
	else if ((buffer[pos + 1] == 0x02) && (buffer[pos + 2] == 0x08) && (buffer[pos + 3] == 0x01))
	{
		SellCounterTotal = getSMLValue(pos);
	}
	else if ((buffer[pos + 1] == 0x10) && (buffer[pos + 2] == 0x07) && (buffer[pos + 3] == 0x00))
	{
		currentPower = getSMLValue(pos);
		addLog(LOG_LEVEL_DEBUG, "currentPower found");
		Serial.print("currentPower "); Serial.println(currentPower);
	}
	return(pos);
}


unsigned int  MT681::crcReflectw(uint16_t data) {
	word ret;
	byte i;

	ret = data & 0x01;
	for (i = 1; i < 16; i++) {
		data >>= 1;
		ret = (ret << 1) | (data & 0x01);
	}
	return ret;
}
unsigned char MT681::crcReflectb(uint8_t data) {
	word ret;
	byte i;

	ret = data & 0x01;
	for (i = 1; i < 8; i++) {
		data >>= 1;
		ret = (ret << 1) | (data & 0x01);
	}
	return ret;
}

unsigned int MT681::crcX25(uint8_t* data, uint16_t data_len) {
	const byte *d = (const byte *)data;
	byte i, c;
	word bit;
	word crc = 0x84cf;

	while (data_len--) {
		yield();
		c = crcReflectb(*d++);
		for (i = 0; i < 8; i++) {
			bit = crc & 0x8000;
			crc = (crc << 1) | ((c >> (7 - i)) & 0x01);
			if (bit) {
				crc ^= 0x1021;
			}
		}
		crc &= 0xffff;
	}
	crc &= 0xffff;

	for (i = 0; i < 16; i++) {
		bit = crc & 0x8000;
		crc = (crc << 1) | 0x00;
		if (bit) {
			crc ^= 0x1021;
		}
	}
	crc = crcReflectw(crc);
	crc = (crc ^ 0xffff) & 0xffff;
	return crc;
}

unsigned int  MT681::checkCRC(unsigned int frameEnd) {
	uint16_t b = crcX25(&buffer[0], frameEnd);
	uint16_t c = (uint16_t)((uint16_t)(buffer[frameEnd + 1] << 8) + (uint16_t)buffer[frameEnd]);
	if (b != c) { // CRC fail
		addLog(LOG_LEVEL_DEBUG, "crc fail");//
		Serial.println("crc fail");
		return 0;
	}
	else {
		addLog(LOG_LEVEL_DEBUG, "crc OK");//
		Serial.print("crc ok");
		return 1;
	}
}

void MT681::emptyBuffers() {
	while (Serial.available())  Serial.read();
	bufferPtr = 0; // empty RX Software buffer
	RState = WAITFORBEGIN;
	addLog(LOG_LEVEL_DEBUG, "eB");
	return;
}


unsigned int MT681::readSML() {
	//Serial.print("*"); 
	//Serial.println(Serial.available());
	if (!Serial.available()) return(0);  // nothing to receive
	if (Serial.available() > 990) {      // hardware buffer overflow -
		Serial.println("!");
		emptyBuffers(); //empty RX hardware buffer and throw data away
		return(0);
	}


	while (Serial.available()) {
		buffer[bufferPtr++] = Serial.read();
		switch (RState) {
		case WAITFORBEGIN:
			if (buffer[bufferPtr - 1] == FrameStart[bufferPtr - 1]) {
				if (bufferPtr == 8)  RState = WAITFOREND;  // start sequence finished
			}
			else {
				bufferPtr = 0;  // start sequence fail
			}
			break;

		case WAITFOREND:

			if ((buffer[bufferPtr - 2] == FrameEnd[4]) && // current buffPtr byte not yet received, the last one is just padding. The -2 byte ist the last one of the end signature.
				(buffer[bufferPtr - 3] == FrameEnd[3]) &&
				(buffer[bufferPtr - 4] == FrameEnd[2]) &&
				(buffer[bufferPtr - 5] == FrameEnd[1]) &&
				(buffer[bufferPtr - 6] == FrameEnd[0])) {
				RState = GETCHECKSUM;
			}
			break;

		case GETCHECKSUM:
			// one crc byte should be in the buffer now, next loop fetches the 2nd.
			RState = CHECKCRC;
			break;

		case CHECKCRC:
			if (checkCRC(bufferPtr - 2)) {
				unsigned int a;
				a = 0;
				for (int i = 0; i < 12; i++)
					a = findValue(a); // parse the values in the frame.
			}
			emptyBuffers();

			break;

		default:
			emptyBuffers(); // should not get here.  
		} // switch  


		if (bufferPtr >= SML_MSG_BUFFER_SIZE) emptyBuffers(); // error: SW Buffer full.

	} // while
}// end func




#ifndef CONFIG
#define CONFIG(n) (Settings.TaskDevicePluginConfig[event->TaskIndex][n])
#endif

MT681*  Plugin_182_MT681 = NULL;


//==============================================
// PLUGIN
// =============================================

boolean Plugin_182(byte function, struct EventStruct *event, String& string) {
	boolean success = false;

	switch (function)
	{
	case PLUGIN_TEN_PER_SECOND:
	{
		Plugin_182_MT681->readSML();
		break;
	}

	case PLUGIN_SERIAL_IN:
	{
		break;
	}
	case PLUGIN_DEVICE_ADD:
	{
		Device[++deviceCount].Number = PLUGIN_ID_182;
		Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
		Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;
		Device[deviceCount].Ports = 0;
		Device[deviceCount].PullUpOption = false;
		Device[deviceCount].InverseLogicOption = false;
		Device[deviceCount].FormulaOption = true;
		Device[deviceCount].ValueCount = 3;
		Device[deviceCount].SendDataOption = true;
		Device[deviceCount].TimerOption = true;
		Device[deviceCount].GlobalSyncOption = true;
		break;
	}

	case PLUGIN_GET_DEVICENAME:
	{
		string = F(PLUGIN_NAME_182);
		break;
	}

	case PLUGIN_GET_DEVICEVALUENAMES:
	{
		strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_182));
		strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_182));
		strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_182));
		break;
	}

	case PLUGIN_WEBFORM_LOAD:
	{
		success = true;
		break;
	}

	case PLUGIN_WEBFORM_SAVE:
	{
		success = true;
		break;
	}

	case PLUGIN_INIT:
	{
		if (!Settings.UseSerial)
		{

#ifndef ARDUINO_ESP8266_RELEASE_2_3_0
			Serial.setRxBufferSize(750);
#endif
			Serial.flush();
			Serial.begin(9600, SERIAL_8N1, (SerialMode)SERIAL_RX_ONLY);
		}
		if (Plugin_182_MT681)
			delete Plugin_182_MT681;
		Plugin_182_MT681 = new MT681();
		success = true;
		break;
	}
	case PLUGIN_READ:
	{
		if (!Plugin_182_MT681)
			return success;
		UserVar[event->BaseVarIndex + 0] = Plugin_182_MT681->SellCounterTotal;
		UserVar[event->BaseVarIndex + 1] = Plugin_182_MT681->BuyCounterTotal;
		UserVar[event->BaseVarIndex + 2] = Plugin_182_MT681->currentPower;
		String log;
		// log = F("MT681: SellCounter: ");          log += UserVar[event->BaseVarIndex + 0];      addLog(LOG_LEVEL_INFO, log);
		// log = F("MT681: BuyCounter: ");           log += UserVar[event->BaseVarIndex + 1];      addLog(LOG_LEVEL_INFO, log);
		log = F("CP: ");         log += UserVar[event->BaseVarIndex + 2];      addLog(LOG_LEVEL_INFO, log);
		success = true;
		break;
	}
	}
	return success;
}

#endif // testing


