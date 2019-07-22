//#######################################################################################################
//######################## Plugin 177 CM1107 I2C CO2 Sensor ############################################
//#######################################################################################################
// development version
// by: V0JT4


#define PLUGIN_177
#define PLUGIN_ID_177         177
#define PLUGIN_NAME_177       "Gases - CO2 CM1107"
#define PLUGIN_VALUENAME1_177 "CO2"

boolean Plugin_177_init = false;


#define CM1107_ADDR   0x31 // default address

#define  CM1107_CMD_CO2     0x01
#define  CM1107_CMD_ABC     0x10
#define  CM1107_CMD_CAL     0x03
#define  CM1107_CMD_SN      0x1F
#define  CM1107_CMD_VER     0x1E

#define  CM1107_STATUS_INIT 0x00 //0 1 swapped???
#define  CM1107_STATUS_OK   0x01
#define  CM1107_STATUS_ERR  0x02
#define  CM1107_STATUS_OUT  0x03
#define  CM1107_STATUS_NCAL 0x05

#define  CM1107_CAL_ON 0x00
#define  CM1107_CAL_OFF 0x02



boolean plugin_177_begin()
{
  //Wire.begin();   called in ESPEasy framework
  Plugin_177_init = true;
  return(true);
}

boolean plugin_177_setABC(unsigned char abc)
  // Set auto calibration period on CM1107
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Write auto calibration configuration
  Wire.beginTransmission(CM1107_ADDR);
  Wire.write(CM1107_CMD_ABC);
  Wire.write(100); // defined by data-sheet
  if (abc)
  {
    Wire.write(CM1107_CAL_ON);
    Wire.write(abc);
  } else
  {
    Wire.write(CM1107_CAL_OFF);
    Wire.write(7);
  }
  // ? make calibration concentration value configurable
  Wire.write((uint8_t)(400 >> 8));
  Wire.write((uint8_t)400);
  Wire.write(100); // defined by data-sheet
  return (Wire.endTransmission() == 0);
}
/* DEV getABC
uint8_t plugin_177_getABC()
{
  I2C_write8(CM1107_ADDR, CM1107_CMD_ABC);
  delay(500);
  uint8_t abc;
  Wire.requestFrom(CM1107_ADDR, (byte)8);

  String log = F("CM1107: ABC:");
  log += Wire.read(); // expected 0x10
  log += F(" ");
  log += Wire.read(); // expected 100
  log += F(" ");
  abc = Wire.read(); // expected 0 or 2
  log += abc;
  log += F(" ");
  if (abc)
  { //expected read 1-15
    log += Wire.read();
    abc = 0;
  } else
  {
    abc = Wire.read();
    log += abc;
  }
  log += F(" ");
  log += (Wire.read() << 8) | Wire.read();
  log += F(" ");
  log += Wire.read(); // expected 100
  log += F(" ");
  log += Wire.read(); // CS
  addLog(LOG_LEVEL_INFO,log);
  return abc;
}*/
String plugin_177_getABC()
{
  I2C_write8(CM1107_ADDR, CM1107_CMD_ABC);
  delay(500);
  uint8_t abc; //replace with function parameter?
  uint16_t co2;
  uint8_t checksum;

  Wire.requestFrom(CM1107_ADDR, (byte)8);
  String log = F("CM1107: ABC: ");
  checksum = Wire.read(); // expected CM1107_CMD_ABC
  if (checksum != CM1107_CMD_ABC)
  {
    log += F("CMD_ERROR ");
  }
  checksum += Wire.read(); // expected 100
  abc = Wire.read(); // expected 0 or 2
  checksum += abc;
  if (abc)
  { //expected read 1-15
    abc = Wire.read();
    checksum += abc;
    log += F("OFF, period: ");
    log += abc;
    abc = 0;
  } else
  {
    abc = Wire.read();
    checksum += abc;
    log += F("ON, period: ");
    log += abc;
  }
  log += F(", ppm: ");
  co2 = Wire.read(); // high byte ppm
  checksum += co2;
  co2 = (co2 << 8) | Wire.read(); // low byte ppm
  checksum += (uint8_t)co2;
  log += co2;
  checksum += Wire.read(); // expected 100
  checksum += Wire.read(); // CS
  log += F(", checksum: ");
  if (checksum)
  {
    log += F("ERR");
    log += checksum;
  } else
  {
    log += F("OK");
  }
  addLog(LOG_LEVEL_INFO,log);
  return log;
}

uint16_t plugin_177_getCO2()
  // Retrieve CO2 data in ppm
{
  I2C_write8(CM1107_ADDR, CM1107_CMD_CO2);
  uint16_t co2;
  uint8_t status;
  uint8_t checksum;

  Wire.requestFrom(CM1107_ADDR, (byte)5);
  checksum = Wire.read(); // CM1107_CMD_CO2
  co2 = Wire.read(); // high byte ppm
  checksum += (uint8_t)co2;
  co2 = (co2 << 8) | Wire.read(); // low byte ppm
  checksum += (uint8_t)co2;
  status = Wire.read(); // status
  checksum += status;
  checksum += Wire.read(); //CS

  String log = F("CM1107: CO2 ppm:");
  log += co2;
  log += F(", status: ");
  log += status;
  log += F(", checksum: ");
  if (checksum)
  {
    log += F("ERR");
    log += checksum;
  } else
  {
    log += F("OK");
  }
  addLog(LOG_LEVEL_INFO,log);
  return co2;
}


boolean Plugin_177(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_177;
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
        string = F(PLUGIN_NAME_177);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_177));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormNumericBox(F("ABC period"), F("plugin_177_CM1107_abc"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addUnit(F("days, 0=off"));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        unsigned char abc = getFormItemInt(F("plugin_177_CM1107_abc"));
        if (abc > 15) abc = 15;
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = abc;
        success = true;
        break;
      }
    case PLUGIN_INIT:
      {
        plugin_177_begin();
        success = plugin_177_setABC((uint8_t) Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        I2C_write8(CM1107_ADDR, CM1107_CMD_CO2);
        break;
      }
    case PLUGIN_READ:
      {
        plugin_177_begin();
        uint16_t co2 = plugin_177_getCO2();
        UserVar[event->BaseVarIndex] = co2;
        if (co2 > 5000)
        {
          addLog(LOG_LEVEL_INFO,F("CM1107: Sensor saturated! > 5000 ppm"));
        }
        success = true;
        break;
      }
    case PLUGIN_WRITE:
      {
        if (string.equalsIgnoreCase(F("CMGETABC")))
        {
          success = true;
          SendStatus(event->Source, plugin_177_getABC());
          I2C_write8(CM1107_ADDR, CM1107_CMD_CO2);
        }
        break;
      }
  }
  return success;
}
