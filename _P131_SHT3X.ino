//#######################################################################################################
//#################### Plugin 131: SHT30/SHT31/SHT35 Temp/Humidity Sensor ###############################
//#######################################################################################################

class SHT3X{
public:
	SHT3X(uint8_t address);
	void get(void);
	float cTemp=0;
//	float fTemp=0;
	float humidity=0;

private:
	uint8_t _address;
};

#define PLUGIN_131
#define PLUGIN_ID_131         131
#define PLUGIN_NAME_131       "Temperature & Humidity - SHT3X"
#define PLUGIN_VALUENAME1_131 "Temperature"
#define PLUGIN_VALUENAME2_131 "Humidity"

boolean Plugin_131_init = false;

uint8_t addr=0x45;
SHT3X sht30(addr);

boolean Plugin_131(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_131;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_131);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_131));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_131));
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_131_init = true;
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        if (!Plugin_131_init) {
          String log = F("SHT3X : not yet initialized!");
          addLog(LOG_LEVEL_ERROR, log);
          break;
        }
        sht30.get();
        UserVar[event->BaseVarIndex] = sht30.cTemp;
        UserVar[event->BaseVarIndex+1] = sht30.humidity;
        String log = F("SHT3X: Temperature: ");
        log += UserVar[event->BaseVarIndex];
        log += F("  Humidity: ");
        log += UserVar[event->BaseVarIndex + 1];
        addLog(LOG_LEVEL_INFO, log);
        success = true;
        break;
      }
  }
  return success;
}

SHT3X::SHT3X(uint8_t address)
{
	Wire.begin();
	_address=address;
}

void SHT3X::get()
{
	unsigned int data[6];

	// Start I2C Transmission
	Wire.beginTransmission(_address);
	// Send measurement command
	Wire.write(0x2C);
	Wire.write(0x06);
	// Stop I2C transmission
	Wire.endTransmission();
	delay(500);

	// Request 6 bytes of data
	Wire.requestFrom(_address, 6);

	// Read 6 bytes of data
	// cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
	if (Wire.available() == 6)
	{
	data[0] = Wire.read();
	data[1] = Wire.read();
	data[2] = Wire.read();
	data[3] = Wire.read();
	data[4] = Wire.read();
	data[5] = Wire.read();
	}

	// Convert the data
	cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
//	fTemp = (cTemp * 1.8) + 32;
	humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
}
