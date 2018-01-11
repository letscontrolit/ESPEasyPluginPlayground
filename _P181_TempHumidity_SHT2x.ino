//#######################################################################################################
//################ Plugin 181: SHT25              Temperature and Humidity Sensor (I2C) ###################
//#######################################################################################################

//#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_181
#define PLUGIN_ID_181         181
#define PLUGIN_NAME_181       "Environment - SHT25 [TESTING]"
#define PLUGIN_VALUENAME1_181 "Temperature"
#define PLUGIN_VALUENAME2_181 "Humidity"
#define PLUGIN_VALUENAME3_181 "AbsoluteHumidity"
#define PLUGIN_VALUENAME4_181 "doVent"
//#define PLUGIN_VALUENAME5_181 "DewPoint" // max 4 variables allowed

//==============================================
// SHT2x LIBRARY - SHT2x.h
// =============================================
# ifndef SHT2x_H
# define SHT2x_H

 
typedef enum {
  eTempHoldCmd = 0xE3,
  eRHumidityHoldCmd = 0xE5,
  eTempNoHoldCmd = 0xF3,
  eRHumidityNoHoldCmd = 0xF5,
} HUM_MEASUREMENT_CMD_T;


class SHT2X
{
public:
  SHT2X(uint8_t addr);
  void get(void);
  float tmp=0;
  float hum=0;
  float abshum=0;
  //float dew=0;
  float doVent=0;
  float GetTemperature();
  float GetHumidity();
  float GetAbsoluteHumidity(float t, float h); 
 // float GetDewpoint (float t, float h);
  bool GetDoVent(float t, float h);
  
  
private:
  uint8_t _i2c_device_address;
  uint16_t readSensor(uint8_t command);
  uint8_t SHT2x_CheckCrc(uint8_t *data, uint8_t nbrOfBytes, uint8_t checksum);

};

#endif

//==============================================
// SHT2x LIBRARY - SHT2x.cpp
// =============================================
SHT2X::SHT2X(uint8_t addr)
{
  _i2c_device_address = addr;
  //Wire.begin();   called in ESPEasy framework
/*
  // Set to periodic mode
  Wire.beginTransmission(_i2c_device_address);
  Wire.write(0x20);   // periodic 0.5mps
  Wire.write(0x32);   // repeatability high
  Wire.endTransmission();
*/
}

void SHT2X::get()
{
    tmp = GetTemperature();
    hum = GetHumidity();
    abshum = GetAbsoluteHumidity (tmp,hum);
  //  dew = GetDewpoint(tmp,hum);
    doVent = (float)GetDoVent(tmp,hum);
}
/**********************************************************
* GetHumidity
* Gets the current humidity from the sensor.
*
* @return float - The relative humidity in %RH
* return NaN in case of error.
**********************************************************/
float SHT2X::GetHumidity()
{
  delay(50); // prevent interference from ongoing wifi transmissions  
  double H; 
  uint16_t val = readSensor(eRHumidityNoHoldCmd); 
  if (val) {
    H = (-6.0 + 125.0 / 65536.0 * (double)(val));
    return H;
    }
  else
     return NAN; 
  }
/**********************************************************
* GetTemperature
* Gets the current temperature from the sensor.
*
* @return float - The temperature in Deg C
* return -100 in case of error.
**********************************************************/
float SHT2X::GetTemperature()
{
  uint16_t val = readSensor(eTempNoHoldCmd);
  double T;
  if (val) {
    T = (-46.85 + 175.72 / 65536.0 * (double)(val));    // read sensor and store value in internal variable
    return T;  
    }
  else 
    return NAN;
 }
/**********************************************************
* GetDewPoint
* Calculates the dew point
* does not read the sensor
* @return float - The dew point in Â°C
**********************************************************/
/* max 4 variables per sensor :-(
float  SHT2X::GetDewpoint(float t, float rh)
{
if (isnan(t) || isnan(rh) ) return NAN; 
double H = (log10(rh)-2)/0.4343 + (17.62*t)/(243.12+t);
double Dp = 243.12*H/(17.62-H); // this is the dew point in Celsius
return Dp;
}
 */
/**********************************************************
* GetAbsoluteHumidity
* Gets the absolute humidity from the sensor.
* @return float - The abs humidity in grams/m3
**********************************************************/
float SHT2X::GetAbsoluteHumidity(float t, float rh)
{
  if (isnan(t) || isnan(rh) ) return NAN; 
  double k1 = (17.67*t) / (t+253.5);
  return (6.112*exp(k1) *rh*2.1674) / (273.15+t);
}

/**********************************************************
* GetDoVent
* Gets a recommendation to ventilate the room to prevent mold
* @return true/false
**********************************************************/

bool SHT2X::GetDoVent(float t, float rh){

// nach Isoplethensystem Schimmel http://images.google.de/imgres?imgurl=http%3A%2F%2Fdocplayer.org%2Fdocs-images%2F15%2F53845%2Fimages%2F12-1.jpg&imgrefurl=http%3A%2F%2Fdocplayer.org%2F53845-Leitfaden-zur-ursachensuche-und-sanierung-bei-schimmelpilzwachstum-in-innenraeumen-schimmelpilzsanierungs-leitfaden.html&h=744&w=471&tbnid=XxFIVIeNVxDG2M%3A&vet=1&docid=Ou59tpShUJ6CZM&ei=Ak4tWOiKMIiFgAbgwK-YDg&tbm=isch&iact=rc&uact=3&dur=1097&page=0&start=0&ndsp=52&ved=0ahUKEwjok-jclK_QAhWIAsAKHWDgC-MQMwgeKAMwAw&bih=1073&biw=1600
// Grenze: Formel: y = 0,00008x^4-0,0063x3 + 0,1883x2 - 2,7848x + 94,713
// x= Temperatur, y= Feuchte die nicht ueberschritten werden sollte
 
  if (isnan(t) || isnan(rh) ) return NAN; 
  
  double b; // mold humidity
  b=0.000038720*t*t*t*t;
  b-=0.003965321*t*t*t;
  b+=0.149368329*t*t;
  b-=2.596918188*t;
  b+=94.660142724;
  
  if (rh>b) 
    return true; 
  else
    return(false);
}


/**********************************************************
* Commnunications
**********************************************************/
uint16_t SHT2X::readSensor(uint8_t command)
{
	int i = 0;
	uint16_t result;
	uint8_t checksum;
	addLog(LOG_LEVEL_DEBUG, "start SHT25 read");
	unsigned int t = millis();
	while (Wire.available()) Wire.read(); // discard whatever there is 
	Wire.beginTransmission(_i2c_device_address); //begin
	Wire.write(command); //send the pointer location
	delay(1);
	i = Wire.endTransmission(); //end
	if (i) {
		addLog(LOG_LEVEL_ERROR, "Wire.endTransmission fail");
		return(NAN);
	}
	while (Wire.available() < 3 && ((i++) < 21)) {
		delay(5);
		Wire.requestFrom((uint8_t)_i2c_device_address, (uint8_t)3);
	}
	//Store the result
	if (Wire.available() < 3) {
		addLog(LOG_LEVEL_DEBUG, "Sensor <3 bytes");
		return(NAN);
	}
	result = ((Wire.read()) << 8);
	result += Wire.read();
	checksum = Wire.read();

	checksum = SHT2x_CheckCrc((uint8_t*)&result, 2, checksum);
	result &= ~0x0003; // clear two low bits (status bits)
	addLog(LOG_LEVEL_DEBUG, "end Sensor read");
	if (checksum > 0)
		return result;
	else {
		addLog(LOG_LEVEL_ERROR, "SHT25 CRC fail");
		return NAN;
	}
};

/**********************************************************
* Check CRC
**********************************************************/

uint8_t SHT2X::SHT2x_CheckCrc(uint8_t *data, uint8_t nbrOfBytes, uint8_t checksum)
 {
uint8_t crc = 0;
uint8_t byteCtr;
const uint16_t POLYNOMIAL = 0x131; //P(x)=x^8+x^5+x^4+1 = 100110001
//calculates 8-Bit checksum with given polynomial

for (byteCtr = nbrOfBytes;   byteCtr >0; byteCtr--)
  { 
  crc ^= (data[byteCtr-1]);
  for (uint8_t bit = 0; bit <8; bit++)
    { if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
    else crc = (crc << 1); 
    }
  }
 if (crc != checksum) 
  return 0;
 else 
  return 1;
}

 

 



#ifndef CONFIG
#define CONFIG(n) (Settings.TaskDevicePluginConfig[event->TaskIndex][n])
#endif

SHT2X*  Plugin_181_SHT2x = NULL;


//==============================================
// PLUGIN
// =============================================

boolean Plugin_181(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number = PLUGIN_ID_181;
      Device[deviceCount].Type = DEVICE_TYPE_I2C;
      Device[deviceCount].VType = SENSOR_TYPE_QUAD;
      Device[deviceCount].Ports = 0;
      Device[deviceCount].PullUpOption = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption = true;
      Device[deviceCount].ValueCount = 4;
      Device[deviceCount].SendDataOption = true;
      Device[deviceCount].TimerOption = true;
      Device[deviceCount].GlobalSyncOption = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_181);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_181));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_181));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_181));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_181));
    //  strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[4], PSTR(PLUGIN_VALUENAME5_181));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      int optionValues[1] = { 0x40 };
      addFormSelectorI2C(string, F("i2c_addr"), 1, optionValues, CONFIG(0));

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      CONFIG(0) = getFormItemInt(F("i2c_addr"));

      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      if (Plugin_181_SHT2x)
        delete Plugin_181_SHT2x;
      Plugin_181_SHT2x = new SHT2X(CONFIG(0));
      //Wire.begin();
       Wire.beginTransmission(0x40); // address the SHT25
       Wire.write(0xFE); // 0xFE is the soft reset command
       Wire.endTransmission(true); // send + stop
       delay(15);

      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      if (!Plugin_181_SHT2x)
        return success;

      Plugin_181_SHT2x->get();
      UserVar[event->BaseVarIndex + 0] = Plugin_181_SHT2x->tmp;
      UserVar[event->BaseVarIndex + 1] = Plugin_181_SHT2x->hum;
      UserVar[event->BaseVarIndex + 2] = Plugin_181_SHT2x->abshum;
      UserVar[event->BaseVarIndex + 3] = Plugin_181_SHT2x->doVent;
     //UserVar[event->BaseVarIndex + 4] = Plugin_181_SHT2x->dew; // drop this one : vars per task is limited to 4
       String log = F("SHT2x: Temperature: ");
      log += UserVar[event->BaseVarIndex + 0];
      addLog(LOG_LEVEL_INFO, log);
      log = F("SHT2x: Humidity: ");
      log += UserVar[event->BaseVarIndex + 1];
      addLog(LOG_LEVEL_INFO, log);

      log = F("SHT2x: AbsoluteHumidity: ");
      log += UserVar[event->BaseVarIndex + 2];
      addLog(LOG_LEVEL_INFO, log);

     log = F("SHT2x: doVent: ");
      log += UserVar[event->BaseVarIndex + 3];
      addLog(LOG_LEVEL_INFO, log);

      success = true;
      break;
    }
  }
  return success;
}

//#endif // testing

