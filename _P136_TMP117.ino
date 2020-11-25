#ifdef PLUGIN_BUILD_TESTING

//#######################################################################################################
//######################## Plugin 136 TMP117 I2C high-accuracy Temperature Sensor ############################################
//#######################################################################################################
// by tonydee ### 20-03-2020 https://microtechitalia.it
#include <TMP117.h>

#define PLUGIN_136
#define PLUGIN_ID_136        136
#define PLUGIN_NAME_136       "Temperature High Precision - TMP117 [TESTING]"
#define PLUGIN_VALUENAME1_136 "Temperature"

// ======================================
// TMP117 I2C high-accuracy Temperature Sensor
// ======================================

boolean Plugin_136_init = false;
// Select the correct address setting
uint8_t ADDR_GND =  0x48;   // 1001000 
uint8_t ADDR_VCC =  0x49;   // 1001001
uint8_t ADDR_SDA =  0x4A;   // 1001010
uint8_t ADDR_SCL =  0x4B;   // 1001011
uint8_t ADDR =  ADDR_GND;
uint8_t ConvTime = 0x00;
uint8_t Averaging = 0x00;

boolean Plugin_136(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
   
  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_136;
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
        string = F(PLUGIN_NAME_136);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_136));
      //  strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_136));
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

        string += F("<TR><TD>ConvTime:<TD><select name='plugin_136_ConvTime'>");
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
        
        string += F("<TR><TD>Averaging:<TD><select name='plugin_136_Averaging'>");
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
        string += F("plugin_136_Adjust' value='");
        string += Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2];
        string += F("'>");
        string += F("<TR><TD>Offset [&#176;C]:<TD><input type='text' title='set temperature offset like ' name='");
        string += F("plugin_136_Offset' value='");
        string += Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3];
        string += F("'>");
          LoadTaskSettings(event->TaskIndex); // we need to restore our original taskvalues!
    
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg F("plugin_136_ConvTime");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        String plugin2 = WebServer.arg F("plugin_136_Averaging");
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();
        String TMP117_adjust = WebServer.arg F("plugin_136_Adjust");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2] = TMP117_adjust.toFloat();
        String TMP117_offset = WebServer.arg F("plugin_136_Offset");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3] = TMP117_offset.toFloat();
        
        Plugin_136_init = false; // Force device setup next time
        success = true;
        break;
      }
         
   case PLUGIN_INIT:
    {
      LoadTaskSettings(event->TaskIndex); // we need to restore our original taskvalues!
      Plugin_136_init = true;
      success = true;
      break;
  }
  
    case PLUGIN_READ:
      {
      TMP117 TMP117_tmp(ADDR);    // I2C address for the sensor
      //float TMP117_offset = 0.0;
      //float TMP117_adjust = 0.5;
         
        // Get sensor resolution configuration
         uint8_t ConvTime = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
         uint8_t Averaging = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
         float TMP117_adjust = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2];
         float TMP117_offset = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3];
        
         Wire.begin();           
  /* The default setup is :
   *    Conversion mode = CONTINUOUS  ---> continuoss
   *    Conversion time = C125mS      -|
   *    Averaging mode  = AVE8        -|-> new data every 125mS
   *    Alert mode      = data        ---> alert pin states that new data is available
   *    
   */
  TMP117_tmp.init ( NULL );                // no callback
  TMP117_tmp.setConvMode (CONTINUOUS);     // contious measurement, also ONESHOT or SHUTWDOWN possible

  switch (ConvTime)
        {
        case (0x00):
          TMP117_tmp.setConvTime (C15mS5);         // 1. setup C125mS+NOAVE = 15.5 mS measurement time
          break;
        case (0x01):
          TMP117_tmp.setConvTime (C125mS);         
          break;
        case (0x02):
          TMP117_tmp.setConvTime (C250mS);
          break;
        case (0x03):
          TMP117_tmp.setConvTime (C500mS);
          break;
        case (0x04):
          TMP117_tmp.setConvTime (C1S);
          break;
        case (0x05):
          TMP117_tmp.setConvTime (C4S);
          break;
        case (0x06):
          TMP117_tmp.setConvTime (C8S);
          break;
        case (0x07):
          TMP117_tmp.setConvTime (C16S);
          break; 
        default:
          TMP117_tmp.setConvTime (C15mS5); 
         break;  
        }

  switch (Averaging)
        {
        case (0x00):
          TMP117_tmp.setAveraging (NOAVE); 
          break;
        case (0x01):
          TMP117_tmp.setAveraging (AVE8);         
          break;
        case (0x02):
          TMP117_tmp.setAveraging (AVE32);
          break;
        case (0x03):
          TMP117_tmp.setAveraging (AVE64);
          break;
        default:
          TMP117_tmp.setAveraging (NOAVE);
         break;
        }       

          TMP117_tmp.setOffsetTemperature ( TMP117_offset );      //set temperature offset
          TMP117_tmp.setTargetTemperature ( TMP117_adjust );      //calibrate sensor to target temperature 
          TMP117_tmp.update();

           // double temperature = TMP117_tmp.getTemperature();
            UserVar[event->BaseVarIndex] = TMP117_tmp.getTemperature();
            String log = F("TMP117: Temperature: ");
            log += UserVar[event->BaseVarIndex];
            addLog(LOG_LEVEL_INFO, log);
            
            success = true;
        break;
      }
  }
  return success;
}
    
#endif
