//#######################################################################################################
//######################## Plugin 176 CDM7160 I2C CO2 Sensor ############################################
//#######################################################################################################
// development version
// by: V0JT4


#define PLUGIN_176
#define PLUGIN_ID_176        176
#define PLUGIN_NAME_176       "Gases - CO2 CDM7160"
#define PLUGIN_VALUENAME1_176 "CO2"
#define PLUGIN_VALUENAME2_176 "BUSY"

boolean Plugin_176_init = false;


#define CDM7160_ADDR   0x69 // default address
#define CDM7160_ADDR_0 0x68 // CAD0 tied to GND

#define  CDM7160_REG_RESET   0x00
#define  CDM7160_REG_CTL     0x01
#define  CDM7160_REG_STATUS  0x02
#define  CDM7160_REG_DATA    0x03
#define  CDM7160_REG_HIT     0x0A
#define  CDM7160_REG_FUNC    0x0F

#define  CDM7160_FLAG_REST  0x01
#define  CDM7160_FLAG_DOWN  0x00
#define  CDM7160_FLAG_CONT  0x06
#define  CDM7160_FLAG_BUSY  0x80
#define  CDM7160_FLAG_HPAE  0x04
#define  CDM7160_FLAG_PWME  0x01


byte plugin_176_i2caddr;

boolean plugin_176_begin()
{
  Wire.begin();
  return(true);
}

boolean plugin_176_readByte(unsigned char address, unsigned char &value)
  // Reads a byte from a CDM7160 address
  // Address: CDM7160 address (0 to 18)
  // Value will be set to stored byte
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Set up command byte for read
  Wire.beginTransmission(plugin_176_i2caddr);
  Wire.write(address & 0x1F);

  // Read requested byte
  if (!Wire.endTransmission())
  {
    Wire.requestFrom(plugin_176_i2caddr,(byte)1);
    if (Wire.available() == 1)
    {
      value = Wire.read();
      return(true);
    }
  }
  return(false);
}

boolean plugin_176_writeByte(unsigned char address, unsigned char value)
  // Write a byte to a CDM7160 address
  // Address: CDM7160 address (0 to 18)
  // Value: byte to write to address
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Set up command byte for write
  Wire.beginTransmission(plugin_176_i2caddr);
  Wire.write(address & 0x1F);
  // Write byte
  Wire.write(value);
  if (!Wire.endTransmission())
    return(true);

  return(false);
}

boolean plugin_176_readUInt(unsigned char address, unsigned int &value)
  // Reads an unsigned integer (16 bits) from a CDM7160 address (low byte first)
  // Address: CDM7160 address (0 to 18), low byte first
  // Value will be set to stored unsigned integer
  // Returns true (1) if successful, false (0) if there was an I2C error
{

  // Set up command byte for read
  Wire.beginTransmission(plugin_176_i2caddr);
  Wire.write(address & 0x1F);
  
  // Read two bytes (low and high)
  if (!Wire.endTransmission())
  {
    Wire.requestFrom(plugin_176_i2caddr,(byte)2);
    if (Wire.available() == 2)
    {
      char high, low;
      low = Wire.read();
      high = Wire.read();
      // Combine bytes into unsigned int
      value = word(high,low);
      return(true);
    }
  }
  return(false);
}

boolean plugin_176_setPowerDown(void)
  // Set power down mode CDM7160 to enable correct settings modification
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Write 0x06 to control register
  return(plugin_176_writeByte(CDM7160_REG_CTL,CDM7160_FLAG_DOWN));
}

boolean plugin_176_setContinuous(void)
  // Set continuous operation mode CDM7160 to start measurements
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Write 0x00 to control register
  return(plugin_176_writeByte(CDM7160_REG_CTL,CDM7160_FLAG_CONT));
}

boolean plugin_176_setReset(void)
  // Reset CDM7160
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Write 0x01 to reset register
  return(plugin_176_writeByte(CDM7160_REG_RESET,CDM7160_FLAG_REST));
}

boolean plugin_176_setAltitude(unsigned char alt)
  // Set altitude compensation on CDM7160
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Write altitude and enable compensation
  plugin_176_writeByte(CDM7160_REG_HIT,alt);
  delay(100);
  return(plugin_176_writeByte(CDM7160_REG_FUNC,(CDM7160_FLAG_HPAE | CDM7160_FLAG_PWME)));
}

boolean plugin_176_clearAltitude(void)
  // Disable altitude compensation on CDM7160
  // Returns true (1) if successful, false (0) if there was an I2C error
{
  // Clear altitude and disable compensation
  plugin_176_writeByte(CDM7160_REG_HIT,0);
  delay(100);
  return(plugin_176_writeByte(CDM7160_REG_FUNC, CDM7160_FLAG_PWME));
}

boolean plugin_176_getStatus(unsigned char &status)
  // Retrieve CO2 data in ppm
  // Returns true (1) if successful, false (0) if there was an I2C error
  // (Also see getError() below)
{
  // Get content of status register
  if (plugin_176_readByte(CDM7160_REG_STATUS, status))
    return(true);

  return(false);
}

boolean plugin_176_getCO2(unsigned int &co2)
  // Retrieve CO2 data in ppm
  // Returns true (1) if successful, false (0) if there was an I2C error
  // (Also see getError() below)
{
  // Get co2 ppm data out of result registers
  if (plugin_176_readUInt(CDM7160_REG_DATA, co2))
    return(true);

  return(false);
}


boolean Plugin_176(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_176;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
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
        string = F(PLUGIN_NAME_176);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_176));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_176));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice1 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];

        int optionValues1[2];
        optionValues1[0] = CDM7160_ADDR;
        optionValues1[1] = CDM7160_ADDR_0;
    addFormSelectorI2C(string, F("plugin_176_cdm7160_i2c"), 2, optionValues1, choice1);
    
    addFormNumericBox(string, F("Altitude"), F("plugin_176_cdm7160_elev"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
    addUnit(string, F("m"));
    success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_176_cdm7160_i2c"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_176_cdm7160_elev"));
        success = true;
        break;
      }
  case PLUGIN_INIT:
      {
      plugin_176_setPowerDown(); // disabled because there were issues setting alt in powerdown mode
      unsigned char elev = Settings.TaskDevicePluginConfig[event->TaskIndex][1] / 10;
      delay(100);
      if (elev)
      {
       plugin_176_setAltitude(elev);
      } else
      {
       plugin_176_clearAltitude();
      }
      delay(100);
      plugin_176_setContinuous();
      success = true;
      }
    case PLUGIN_READ:
      {
        plugin_176_i2caddr = Settings.TaskDevicePluginConfig[event->TaskIndex][0];

        plugin_176_begin();
         unsigned int co2;
     unsigned char status;
     
     unsigned char count;
     for (count = 0; count < 6; count++)
     {
      if (plugin_176_getStatus(status))
      {
        if (status & CDM7160_FLAG_BUSY)
        {
          delay(50); // BUSY may take up to 300ms
        } else
        {
          break;
        }
      }
     }
     UserVar[event->BaseVarIndex + 1] = count;
          
         if (plugin_176_getCO2(co2))
         {
           boolean good;  // True if sensor is not saturated
           good = (co2 <= 10000);
           UserVar[event->BaseVarIndex] = co2;
           if (!good)
           {
             addLog(LOG_LEVEL_INFO,F("CDM7160: Sensor saturated! > 10000 ppm"));
           }
           success = true;
           String log = F("CDM7160: Address: 0x");
           log += String(plugin_176_i2caddr,HEX);
           log += F(": CO2 ppm: ");
           log += UserVar[event->BaseVarIndex];
       log += F(", busy: ");
           log += UserVar[event->BaseVarIndex + 1];
       log += F(", alt: ");
       plugin_176_readByte(CDM7160_REG_HIT,status);
           log += status;
       log += F(", comp: ");
       plugin_176_readByte(CDM7160_REG_FUNC,status);
           log += status;
           addLog(LOG_LEVEL_INFO,log);
         }
         else
         {
           // getData() returned false because of an I2C error, inform the user.
           addLog(LOG_LEVEL_ERROR, F("CDM7160 READ: i2c error"));

         }
        break;
      }
  case PLUGIN_WRITE:
      {
        if (string.equalsIgnoreCase(F("CDMRST")))
        {
          success = true;
          addLog(LOG_LEVEL_INFO, F("CDM7160: reset"));
      plugin_176_setReset();
        }
        break;
      }
  }
  return success;
}
