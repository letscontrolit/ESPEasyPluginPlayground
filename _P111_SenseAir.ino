//#######################################################################################################
//############################# Plugin 111: SenseAir CO2 Sensors ########################################
//#######################################################################################################
/*
  Plugin originally written by: Daniel Tedenljung info__AT__tedenljungconsulting.com
  Rewritten by: Mikael Trieb

  This plugin reads availble values of SenseAir Co2 Sensors.
  Datasheet can be found here:
  S8: http://www.senseair.com/products/oem-modules/senseair-s8/
  K30: http://www.senseair.com/products/oem-modules/k30/
  K70/tSENSE: http://www.senseair.com/products/wall-mount/tsense/

  You can buy sensor from m.nu in Sweden:
  S8 https://www.m.nu/co2matare-fran-senseair-p-1440.html
  K30 https://www.m.nu/k30-co2matare-p-302.html
*/


#define PLUGIN_111
#define PLUGIN_ID_111         111
#define PLUGIN_NAME_111       "SenseAir CO2 (Temperature & Humidity) Sensor"
#define PLUGIN_VALUENAME1_111 "Carbon Dioxide"
#define PLUGIN_VALUENAME2_111 "Temperature"
#define PLUGIN_VALUENAME3_111 "Humidity"

boolean Plugin_111_init = false;

#include <SoftwareSerial.h>
SoftwareSerial *Plugin_111_SoftSerial;

// 0xFE=Any address 0x04=Read input registers 0x0003=Starting address 0x0001=Number of registers to read 0xD5C5=CRC in reverse order
byte cmdReadPPM[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};

byte ReciveBuffer[7];

byte Data[5];
byte co2[2];
long ppm;

boolean Plugin_111(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_111;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
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
        string = F(PLUGIN_NAME_111);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_111));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_111));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_111));
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_111_init = true;
        Plugin_111_SoftSerial = new SoftwareSerial(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex]); // TODO: Explain this in plugin description RX=GPIO Setting 1, TX=GPIO Setting 2, Use 1kOhm in serie on datapins!
        success = true;
        break;
      }

    case PLUGIN_READ:
      {

        if (Plugin_111_init)
        {

          int sensor_status = Plugin_111_readStatus();
          if(sensor_status == 0){
            int co2 = Plugin_111_readCo2();
            UserVar[event->BaseVarIndex] = co2;

            String log = F("SenseAir - Co2: ");
            log += co2;
            addLog(LOG_LEVEL_INFO, log);

            // The following values only exist for K70
            float temperature = Plugin_111_readTemperature();
            UserVar[event->BaseVarIndex + 1] = (float)temperature;
            log = F("SenseAir - Temperature: ");
            log += temperature;
            addLog(LOG_LEVEL_INFO, log);

            float relativeHumidity = Plugin_111_readRelativeHumidity();
            UserVar[event->BaseVarIndex + 2] = (float)relativeHumidity;

            log = F("SenseAir - Humidity: ");
            log += relativeHumidity;
            addLog(LOG_LEVEL_INFO, log);
          }
          else{
            String log = F("SenseAir - Status error:");
            log += sensor_status;
            addLog(LOG_LEVEL_INFO, log);
            UserVar[event->BaseVarIndex] = NAN;
            UserVar[event->BaseVarIndex + 1] = NAN;
            UserVar[event->BaseVarIndex + 2] = NAN;
          }

          success = true;
          break;
        }
        break;
      }
  }
  return success;
}

void Plugin_111_buildFrame(byte slaveAddress,
              byte  functionCode,
              short startAddress,
              short numberOfRegisters,
              byte frame[8])
{
  frame[0] = slaveAddress;
  frame[1] = functionCode;
  frame[2] = (byte)(startAddress >> 8);
  frame[3] = (byte)(startAddress);
  frame[4] = (byte)(numberOfRegisters >> 8);
  frame[5] = (byte)(numberOfRegisters);
  // CRC-calculation
  byte checkSum[2] = {0};
  unsigned int crc = Plugin_111_ModRTU_CRC(frame, 6, checkSum);
  frame[6] = checkSum[0];
  frame[7] = checkSum[1];
}


int Plugin_111_sendCommand(byte command[])
{
  byte recv_buf[7] = {0xff};
  byte data_buf[2] = {0xff};
  long value       = -1;

  Plugin_111_SoftSerial->write(command, 8); //Send the byte array
  delay(50);

  // Read answer from sensor
  int ByteCounter = 0;
  while(Plugin_111_SoftSerial->available()) {
    recv_buf[ByteCounter] = Plugin_111_SoftSerial->read();
    //Serial.print(recv_buf[ByteCounter], HEX);Serial.print(",");
    ByteCounter++;
  }

  data_buf[0] = recv_buf[3];
  data_buf[1] = recv_buf[4];
  value = (data_buf[0] << 8) | (data_buf[1]);

  return value;
}

int Plugin_111_readStatus(void)
{
  int sensor_status = -1;
  byte frame[8] = {0};
  Plugin_111_buildFrame(0xFE, 0x04, 0x00, 1, frame);
  sensor_status = Plugin_111_sendCommand(frame);
  return sensor_status;
}

int Plugin_111_readCo2(void)
{
  int co2 = 0;
  byte frame[8] = {0};
  Plugin_111_buildFrame(0xFE, 0x04, 0x03, 1, frame);
  co2 = Plugin_111_sendCommand(frame);
  return co2;
}

float Plugin_111_readTemperature(void)
{
  int temperatureX100 = 0;
  float temperature = 0.0;
  byte frame[8] = {0};
  Plugin_111_buildFrame(0xFE, 0x04, 0x04, 1, frame);
  temperatureX100 = Plugin_111_sendCommand(frame);
  temperature = temperatureX100/100;
  return temperature;
}

float Plugin_111_readRelativeHumidity(void)
{
  int rhX100 = 0;
  float rh = 0.0;
  byte frame[8] = {0};
  Plugin_111_buildFrame(0xFE, 0x04, 0x05, 1, frame);
  rhX100 = Plugin_111_sendCommand(frame);
  rh = rhX100/100;
  return rh;
}


// Compute the MODBUS RTU CRC
unsigned int Plugin_111_ModRTU_CRC(byte buf[], int len, byte checkSum[2])
{
  unsigned int crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];          // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  checkSum[1] = (byte)((crc >> 8) & 0xFF);
  checkSum[0] = (byte)(crc & 0xFF);
  return crc;
}
