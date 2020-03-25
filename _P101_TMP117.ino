#ifdef PLUGIN_BUILD_TESTING

//#######################################################################################################
//######################## Plugin 101 TMP117 I2C high-accuracy Temperature Sensor ############################################
//#######################################################################################################
// by tonydee ### 20-03-2020 Microtech (r), see my projects and blog at https://microtechitalia.it
#include <TMP117.h>

#define PLUGIN_101
#define PLUGIN_ID_101        101
#define PLUGIN_NAME_101       "Temperature High Precision - TMP117 [TESTING]"
#define PLUGIN_VALUENAME1_101 "Temperature"

// ======================================
// TMP117 I2C high-accuracy Temperature Sensor
// ======================================

boolean Plugin_101_init = false;
// Select the correct address setting
uint8_t ADDR_GND =  0x48;   // 1001000 
uint8_t ADDR_VCC =  0x49;   // 1001001
uint8_t ADDR_SDA =  0x4A;   // 1001010
uint8_t ADDR_SCL =  0x4B;   // 1001011
uint8_t ADDR =  ADDR_GND;
uint8_t ConvTime = 0x00;
uint8_t Averaging = 0x00;
float offset = 0.0;
float adjust = 0.5;

TMP117 tmp(ADDR);           // I2C address for the sensor

boolean Plugin_101(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
   
  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_101;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE; 
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;             //number of output variables. The value should match the number of keys PLUGIN_VALUENAME1_xxx
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

       case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_101);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_101));
      //  strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_101));
        break;
      }


    case PLUGIN_WEBFORM_LOAD:
      {
        #define TMP117_ConvTime_OPTION 8
        #define TMP117_Averaging_OPTION 4

        byte choice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options[TMP117_ConvTime_OPTION];
        uint optionValues[TMP117_ConvTime_OPTION];
        optionValues[0] = (0x00);
        options[0] = F("0 - ConvTime C15mS5");
        optionValues[1] = (0x01);
        options[1] = F("1 - ConvTime C125mS");
        optionValues[2] = (0x02);
        options[2] = F("2 - ConvTime C250mS");
        optionValues[3] = (0x03);
        options[3] = F("3 - ConvTime C500mS");
        optionValues[4] = (0x04);
        options[4] = F("4 - ConvTime C1S");
        optionValues[5] = (0x05);
        options[5] = F("5 - ConvTime C4S");
        optionValues[6] = (0x06);
        options[6] = F("6 - ConvTime C8S");
        optionValues[7] = (0x07);
        options[7] = F("7 - ConvTime C16S");

        string += F("<TR><TD>ConvTime:<TD><select name='plugin_101_ConvTime'>");
        for (byte x = 0; x < TMP117_ConvTime_OPTION; x++)
        {
          string += F("<option value='");
          string += optionValues[x];
          string += "'";
          if (choice == optionValues[x])
            string += F(" selected");
          string += ">";
          string += options[x];
          string += F("</option>");
        }
        string += F("</select>");

        byte choice1 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String options1[TMP117_Averaging_OPTION];
        uint optionValues1[TMP117_Averaging_OPTION];
        optionValues1[0] = (0x00);
        options1[0] = F("0 - Averaging (0)");
        optionValues1[1] = (0x01);
        options1[1] = F("1 - Averaging (8)");
        optionValues1[2] = (0x02);
        options1[2] = F("2 - Averaging (32)");
        optionValues1[3] = (0x03);
        options1[3] = F("3 - Averaging (64)");
        
        string += F("<TR><TD>Averaging:<TD><select name='plugin_101_Averaging'>");
        for (byte x = 0; x < TMP117_Averaging_OPTION; x++)
        {
          string += F("<option value='");
          string += optionValues1[x];
          string += "'";
          if (choice1 == optionValues1[x])
            string += F(" selected");
          string += ">";
          string += options1[x];
          string += F("</option>");
        }
        string += F("</select>");
        string += F("<TR><TD>Sensor Adjustment [&#176;C]:<TD><input type='text' title='calibrate sensor to target temperature' name='");
        string += F("plugin_101_Adjust' value='");
        string += Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2];
        string += F("'>");
        string += F("<TR><TD>Offset [&#176;C]:<TD><input type='text' title='set temperature offset like ' name='");
        string += F("plugin_101_Offset' value='");
        string += Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3];
        string += F("'>");
        
       // LoadTaskSettings(event->TaskIndex); // we need to restore our original taskvalues!
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg("plugin_101_ConvTime");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        String plugin2 = WebServer.arg("plugin_101_Averaging");
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();
        String adjust = WebServer.arg("plugin_101_Adjust");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2] = adjust.toFloat();
        String offset = WebServer.arg("plugin_101_Offset");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3] = offset.toFloat();
        
        Plugin_101_init = false; // Force device setup next time
        success = true;
        break;
      }
         
   case PLUGIN_INIT:
    {
      Plugin_101_init = true;
      success = true;
      break;
  }
  
    case PLUGIN_READ:
      {
 /*  command   parameter   description
   *    0           X           print actual temperature
   *    1           X           print EEPROM NIST UUID [E1|E2|E3]
   *    2         float         set temperature offset like "2 20.5" sets the offset to 20.5°C
   *    3         float         calibrate sensor to target temperature like "3 30.5" will calibrate sensor to 30.5°C
   * 
   */
        // Get sensor resolution configuration
         uint8_t ConvTime = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
         uint8_t Averaging = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
         float adjust = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2];
         float offset = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3];
         
        // Initiate wire library and serial communication
  Wire.begin();           
  // Serial.begin(115200);
  /* The default setup is :
   *    Conversion mode = CONTINUOUS  ---> continuoss
   *    Conversion time = C125mS      -|
   *    Averaging mode  = AVE8        -|-> new data every 125mS
   *    Alert mode      = data        ---> alert pin states that new data is available
   *    
   */
  tmp.init ( NULL );                // no callback
  tmp.setConvMode (CONTINUOUS);     // contious measurement, also ONESHOT or SHUTWDOWN possible

 switch (ConvTime)
        {
        case (0x00):
          tmp.setConvTime (C15mS5);         // 1. setup C125mS+NOAVE = 15.5 mS measurement time
          break;
        case (0x01):
          tmp.setConvTime (C125mS);         
          break;
        case (0x02):
          tmp.setConvTime (C250mS);
          break;
        case (0x03):
          tmp.setConvTime (C500mS);
          break;
        case (0x04):
          tmp.setConvTime (C1S);
          break;
        case (0x05):
          tmp.setConvTime (C4S);
          break;
        case (0x06):
          tmp.setConvTime (C8S);
          break;
        case (0x07):
          tmp.setConvTime (C16S);
          break;   
        }

 switch (Averaging)
        {
        case (0x00):
          tmp.setAveraging (NOAVE); 
          break;
        case (0x01):
          tmp.setAveraging (AVE8);         
          break;
        case (0x02):
          tmp.setAveraging (AVE32);
          break;
        case (0x03):
          tmp.setAveraging (AVE64);
          break;
        }       

          tmp.setOffsetTemperature ( offset );      //set temperature offset
          tmp.setTargetTemperature ( adjust );      //calibrate sensor to target temperature 
          tmp.update();

            double temperature = tmp.getTemperature();
            UserVar[event->BaseVarIndex] = temperature;
                                         //  UserVar[event->BaseVarIndex + 1] = humidity;
            String log = F("TMP117: Temperature: ");
            log += UserVar[event->BaseVarIndex];
            addLog(LOG_LEVEL_INFO, log);
            
            success = true;
      if(!tmp.getDeviceID())
        {
         success = false;
        }
      if(!success)
        {
          String log = F("TMP117: No reading!");
          addLog(LOG_LEVEL_INFO, log);
          UserVar[event->BaseVarIndex] = NAN;
        }
        break;
      }
  }
  return success;
}
    
#endif
