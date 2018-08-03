//#######################################################################################################
//######################### Plugin 184: GY_US42V2 Ultrasonic range finder sensor ############################
//#######################################################################################################

#define PLUGIN_184
#define PLUGIN_ID_184                         184
#define PLUGIN_NAME_184                       "Ultrasonic range finder - GY-US42V2"
#define PLUGIN_VALUENAME1_184                 "DISTANCE"

#define GY_US42V2_ADDRESS                     (0x70)  // default address (0x70 = datasheet address 0xE0)
#define GY_US42V2_CMD_RANGE_COMMAND           (0x51)

boolean Plugin_184(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_184;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_184);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_184));
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
        Plugin_184_begin();
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        uint16_t value;
        value = Plugin_184_getDistance();        
        UserVar[event->BaseVarIndex] = value;
        String log = F("P184 : distance = ");
        log += value;
        log += F(" cm");
        addLog(LOG_LEVEL_INFO,log);
        success = true;
        break;
      }
  }
  return success;
}

//**************************************************************************/
// Sensor setup
//**************************************************************************/
void Plugin_184_begin(void) 
{
}

//**************************************************************************/
// Report distance
//**************************************************************************/
uint16_t Plugin_184_getDistance() 
{
  uint16_t value = 0;
    
  Wire.beginTransmission(GY_US42V2_ADDRESS);
  Wire.write(GY_US42V2_CMD_RANGE_COMMAND);
  Wire.endTransmission();
  
  delay(70);                                                                      // transmit -> receive turnaround time (up to 65ms)

  Wire.requestFrom(GY_US42V2_ADDRESS, 2);
  if (Wire.available() > 1) {
    value = ((Wire.read() << 8) | Wire.read());
  }
  
  return value;
}
