//#######################################################################################################
//########################### Plugin 130 VEML6075 I2C UVA/UVB Sensor      ###############################
//#######################################################################################################
//###################################### stefan@clumsy.ch      ##########################################
//#######################################################################################################


#ifdef PLUGIN_BUILD_TESTING


#define PLUGIN_130
#define PLUGIN_ID_130        130
#define PLUGIN_NAME_130       "Environment - VEML6075 [TESTING]"
#define PLUGIN_VALUENAME1_130 "UVA"
#define PLUGIN_VALUENAME2_130 "UVB"
#define PLUGIN_VALUENAME3_130 "UVIndex"


////////////////////////////
// VEML6075 Command Codes //
////////////////////////////
#define  VEML6075_UV_CONF         0x00 // command codes
#define  VEML6075_UVA_DATA        0x07  // 2 bytes
#define  VEML6075_UVDUMMY_DATA    0x08  
#define  VEML6075_UVB_DATA        0x09  
#define  VEML6075_UVCOMP1_DATA    0x0A  
#define  VEML6075_UVCOMP2_DATA    0x0B  
#define  VEML6075_UV_ID           0x0C  // should retrn 0x26

#define ACoef 3.33
#define BCoef 2.5
#define CCoef 3.66
#define DCoef 2.75
#define UVAresponsivity  0.0011
#define UVBresponsivity  0.00125


enum IT {
  IT_50 = 0,  //   50 ms
  IT_100 = 1,     //  100 ms
  IT_200 = 2,     //  200 ms
  IT_400 = 3,     //  400 ms
  IT_800 = 4      //  800 ms
};

// Specify VEML6075 Integration time
uint8_t IT = IT_100;
uint16_t UVData[5] = {0, 0, 0, 0, 0}; // UVA, Dummy, UVB, UVComp1, UVComp2
float UVAComp, UVBComp, UVIndex;
bool HD = 0;



uint8_t veml6075_i2caddr;
int32_t veml6075_sensorID;
int32_t veml6075_t_fine;

uint8_t Plugin_130_read8(byte reg, bool * is_ok = NULL); // Declaration

boolean Plugin_130_init[2] = {false, false};

boolean Plugin_130(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_130;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
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
        string = F(PLUGIN_NAME_130);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_130));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_130));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_130));
        break;
      }
    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        int optionValues[2] = { 0x10, 0x11 };
        addFormSelectorI2C(string, F("plugin_130_veml6075_i2c"), 2, optionValues, choice);
        addFormNote(string, F("SDO Low=0x10, High=0x11"));

        byte choiceMode2 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String optionsMode2[5];
        optionsMode2[0] = F("VEML6075_IT_50MS");
        optionsMode2[1] = F("VEML6075_IT_100MS");
        optionsMode2[2] = F("VEML6075_IT_200MS");
        optionsMode2[3] = F("VEML6075_IT_400MS");
        optionsMode2[4] = F("VEML6075_IT_800MS");
        int optionValuesMode2[5];
        optionValuesMode2[0] = IT_50;
        optionValuesMode2[1] = IT_100;
        optionValuesMode2[2] = IT_200;
        optionValuesMode2[3] = IT_400;
        optionValuesMode2[4] = IT_800;
        addFormSelector(string, F("Integration Time"), F("plugin_130_veml6075_it"), 5, optionsMode2, optionValuesMode2, choiceMode2);

        byte choiceMode3 = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        String optionsMode3[2];
        optionsMode3[0] = F("VEML6075_NORMAL_DYNAMIC");
        optionsMode3[1] = F("VEML6075_HIGH_DYNAMIC");
        int optionValuesMode3[2];
        optionValuesMode3[0] = 0;
        optionValuesMode3[1] = 1;
        addFormSelector(string, F("Dynamic Setting"), F("plugin_130_veml6075_hd"), 2, optionsMode3, optionValuesMode3, choiceMode3);

        

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_130_veml6075_i2c"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_130_veml6075_it"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("plugin_130_veml6075_hd"));
        Plugin_130_init[Settings.TaskDevicePluginConfig[event->TaskIndex][0]] = false;
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
              
        int idx = Settings.TaskDevicePluginConfig[event->TaskIndex][0] ;
        Plugin_130_init[idx] &= Plugin_130_check(Settings.TaskDevicePluginConfig[event->TaskIndex][0]); // Check id device is present
        
        IT = Settings.TaskDevicePluginConfig[event->TaskIndex][1]; // set Integration Time

        if (!Plugin_130_init[idx])
        {
          Plugin_130_init[idx] = Plugin_130_begin(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        }

          String log = F("VEML6075  : idx: 0x");
          log += String(idx,HEX);
          addLog(LOG_LEVEL_DEBUG, log);
          log = F("VEML6075  : plugin(idx): 0x");
          log += String(Plugin_130_init[idx],HEX);
          addLog(LOG_LEVEL_DEBUG, log);

        if (Plugin_130_init[idx])
        {
          Plugin_130_getUVdata(Settings.TaskDevicePluginConfig[event->TaskIndex][0], UVData);

          // Calculate the UV Index, valid in open air not behind glass!
          UVAComp = (UVData[0] - UVData[1]) - ACoef*(UVData[3] - UVData[1]) - BCoef*(UVData[4] - UVData[1]);
          UVBComp = (UVData[2] - UVData[1]) - CCoef*(UVData[3] - UVData[1]) - DCoef*(UVData[4] - UVData[1]);
          UVIndex = (  (UVBComp*UVBresponsivity) +  (UVAComp*UVAresponsivity)  )/2.;

//float UVASensitivity = 0.93/((float) (IT + 1)); // UVA light sensitivity increases with integration time
//float UVBSensitivity = 2.10/((float) (IT + 1)); // UVB light sensitivity increases with integration time
          log = F("VEML6075  : IT raw: 0x");
          log += String((IT + 1),HEX);
          addLog(LOG_LEVEL_DEBUG, log);

          UserVar[event->BaseVarIndex] = UVData[0]/pow(2,IT-1); // UVA light sensitivity increases linear with integration time
          UserVar[event->BaseVarIndex + 1] = UVData[2]/pow(2,IT-1); // UVB light sensitivity increases linear with integration time
          UserVar[event->BaseVarIndex + 2] = UVIndex;

          log = F("VEML6075: Address: 0x");
          log += String(veml6075_i2caddr,HEX);
          log += F(" / Integration Time: ");
          log += Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          log += F(" / Dynamic Mode: ");
          log += Settings.TaskDevicePluginConfig[event->TaskIndex][2];
          log += F(" / divisor: ");
          log += String(pow(2,IT-1));
          log += F(" / UVA: ");
          log += UserVar[event->BaseVarIndex];
          log += F(" / UVB: ");
          log += UserVar[event->BaseVarIndex + 1];
          log += F(" / UVIndex: ");
          log += UserVar[event->BaseVarIndex + 2];
          addLog(LOG_LEVEL_INFO, log);
          
          success = true;
        }
        break;
      }

  }
  return success;
}

//**************************************************************************/
// Check VEML6075 presence
//**************************************************************************/
bool Plugin_130_check(int a) {
  veml6075_i2caddr = a?a:0x10;
  bool wire_status = false;
  uint16_t deviceID = Plugin_130_getVEML6075ID(a);

  String log = F("VEML6075  : ID: 0x");
  log += String(deviceID, HEX);
  addLog(LOG_LEVEL_DEBUG, log);

  if (deviceID != 0x26) {
      return false;
  } else {
      return true;
  }
}

//**************************************************************************/
// Initialize VEML6075
//**************************************************************************/
bool Plugin_130_begin(int a) {
  if (! Plugin_130_check(a))
    return false;

  Plugin_130_enableVEML6075(a); // initalize sensor
  delay(150);
  
  return true;
}

//===================================================================================================================
//====== Set of useful function to access VEML6075 UV data
//===================================================================================================================

uint16_t Plugin_130_getVEML6075ID(int a)
{
    uint8_t rawData[2] = {0, 0};
    Wire.beginTransmission(a);
    Wire.write(0x0C);        // Command code for reading VEML6075 ID
    Wire.endTransmission(false);  // Send the Tx buffer, but send a restart to keep connection alive

    Wire.requestFrom(a, 2);  // Read two bytes from slave register address 
    uint8_t i = 0;
    while (Wire.available()) 
    {
        rawData[i++] = Wire.read();       // Put read results in the Rx buffer
    }     
    Wire.endTransmission();
    return ((uint16_t) rawData[1] << 8) | rawData[0];
}



uint16_t Plugin_130_getUVdata(int a, uint16_t * destination)
{
    for (int j = 0; j < 5; j++)
    {
    uint8_t rawData[2] = {0, 0};
    Wire.beginTransmission(a);
    Wire.write(VEML6075_UVA_DATA + j);        // Command code for reading UV data channels in sequence
    Wire.endTransmission(false);         // Send the Tx buffer, but send a restart to keep connection alive

    Wire.requestFrom(a, 2);    // Read two bytes from slave register address 
    uint8_t i = 0;
    while (Wire.available()) 
    {
        rawData[i++] = Wire.read();       // Put read results in the Rx buffer
    }     
    Wire.endTransmission();
    destination[j] = ((uint16_t) rawData[1] << 8) | rawData[0]; // 16-bit unsigend integer
    }
 
}

void Plugin_130_enableVEML6075(int a)
{
  String log = F("VEML6075  : IT: 0x");
  log += String(IT<<4, HEX);
  addLog(LOG_LEVEL_DEBUG, log);

  Wire.beginTransmission(a);
  Wire.write(VEML6075_UV_CONF); // Command code for configuration register
  Wire.write((IT << 4)|(HD << 3)); // Bit 3 must be 0, bit 0 is 0 for run and 1 for shutdown, LS Byte
  Wire.write(0x00); // MS Byte
  Wire.endTransmission();
}


#endif
