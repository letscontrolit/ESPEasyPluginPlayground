#ifdef USES_P136

//#######################################################################################################
//######################## Plugin 136 TMP117 I2C high-accuracy Temperature Sensor ############################################
//#######################################################################################################
// by tonydee ### 20-03-2020 Microtech, see my projects and blog at https://microtechitalia.it
#include <TMP117.h>

#define PLUGIN_136
#define PLUGIN_ID_136        136
#define PLUGIN_NAME_136       "Temperature High Precision - TMP117"
#define PLUGIN_VALUENAME1_136 "Temperature"

// ======================================
// TMP117 I2C high-accuracy Temperature Sensor
// ======================================

#include "_Plugin_Helper.h"

boolean Plugin_136_init = false;
// Select the correct address setting
uint8_t ADDR_GND =  0x48;   // 1001000  
uint8_t ADDR_VCC =  0x49;   // 1001001
uint8_t ADDR_SDA =  0x4A;   // 1001010
uint8_t ADDR_SCL =  0x4B;   // 1001011

//TMP117 TMP117_tmp(ADDR_GND);    // I2C address for the sensor
  
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
        break;
      }


    case PLUGIN_WEBFORM_LOAD:
      {
        #define TMP117_ConvTime_OPTION 8
        #define TMP117_Averaging_OPTION 4
        
        byte choice = PCONFIG(0);
     //   String options[4] = { F("0x48"), F("0x49"), F("0x4A"), F("0x4B") }; 
        int optionValues[4] = { ADDR_GND, ADDR_VCC, ADDR_SDA, ADDR_SCL };
        addFormSelectorI2C(F("p136_address"), 4, optionValues, choice);   
        addFormNote(F("GND=0x48, VCC=0x49, SDA=0x4A, SCL=0x4B"));
     
        byte choice1 = PCONFIG(1);                         //Set TMP117_ConvTime_OPTION
        String options1[TMP117_ConvTime_OPTION];
        int optionValues1[TMP117_ConvTime_OPTION];
        optionValues1[0] = 0;
        options1[0] = F("0 - ConvTime C15mS5");
        optionValues1[1] = 1;
        options1[1] = F("1 - ConvTime C125mS");
        optionValues1[2] = 2;
        options1[2] = F("2 - ConvTime C250mS");
        optionValues1[3] = 3;
        options1[3] = F("3 - ConvTime C500mS");
        optionValues1[4] = 4;
        options1[4] = F("4 - ConvTime C1S");
        optionValues1[5] = 5;
        options1[5] = F("5 - ConvTime C4S");
        optionValues1[6] = 6;
        options1[6] = F("6 - ConvTime C8S");
        optionValues1[7] = 7;
        options1[7] = F("7 - ConvTime C16S");
        addFormSelector(F("ConvTime"), F("plugin_136_ConvTime"), TMP117_ConvTime_OPTION, options1, optionValues1, choice1);
        //addUnit(F("bits"));

        byte choice2 =  PCONFIG(2);                        //Set TMP117_Averaging_OPTION
        String options2[TMP117_Averaging_OPTION];
        int optionValues2[TMP117_Averaging_OPTION];
        optionValues2[0] = 0;
        options2[0] = F("0 - Averaging (0)");
        optionValues2[1] = 1;
        options2[1] = F("1 - Averaging (8)");
        optionValues2[2] = 2;
        options2[2] = F("2 - Averaging (32)");
        optionValues2[3] = 3;
        options2[3] = F("3 - Averaging (64)");
        addFormSelector(F("Averaging"), F("plugin_136_Averaging"), TMP117_Averaging_OPTION, options2, optionValues2, choice2);
        //addUnit(F("bits"));
        
        addFormNumericBox(F("Sensor Adjustment :"), F("plugin_136_Adjust"), PCONFIG(3));
        addUnit(F("°C"));

        addFormNumericBox(F("Temperature offset"), F("plugin_136_Offset"), PCONFIG(4));
        addUnit(F("x 1°C - resolution 0.0078"));
        String offsetNote = F("Offset in units of 1 degree within the range of +/-255.98°C");
        addFormNote(offsetNote);

        success = true;
        break;
      
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p136_address"));
        PCONFIG(1) = getFormItemInt(F("plugin_136_ConvTime"));
        PCONFIG(2) = getFormItemInt(F("plugin_136_Averaging"));
        PCONFIG(3) = getFormItemInt(F("plugin_136_Adjust"));
        PCONFIG(4) = getFormItemInt(F("plugin_136_Offset"));
        
        Plugin_136_init = false; // Force device setup next time
        success = true;
        break;
      }
      
 case PLUGIN_INIT:
      {
      // Init sensor
     //  TMP117 TMP117_tmp(PCONFIG(0));
       
           Plugin_136_init = true;
        break;
      }

   //      case PLUGIN_ONCE_A_SECOND:
   //   {
  //      TMP117_tmp.update();
  //      break;
   //   }
      
    case PLUGIN_READ:
      {
        const uint8_t ADDR = PCONFIG(0);
        uint16_t ConvTime = PCONFIG(1);
        uint16_t Averaging = PCONFIG(2);
        float TMP117_adjust = PCONFIG(3);
        float TMP117_offset = PCONFIG(4);

        TMP117 TMP117_tmp(ADDR);    // I2C address for the sensor
        // Get sensor resolution configuration
        Wire.begin();
        TMP117_tmp.setOffsetTemperature ( TMP117_offset );      //set temperature offset
        TMP117_tmp.setTargetTemperature ( TMP117_adjust );      //calibrate sensor to target temperature 
        
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
        case (0):
          TMP117_tmp.setConvTime (C15mS5);         // 1. setup C125mS+NOAVE = 15.5 mS measurement time
          break;
        case (1):
          TMP117_tmp.setConvTime (C125mS);         
          break;
        case (2):
          TMP117_tmp.setConvTime (C250mS);
          break;
        case (3):
          TMP117_tmp.setConvTime (C500mS);
          break;
        case (4):
          TMP117_tmp.setConvTime (C1S);
          break;
        case (5):
          TMP117_tmp.setConvTime (C4S);
          break;
        case (6):
          TMP117_tmp.setConvTime (C8S);
          break;
        case (7):
          TMP117_tmp.setConvTime (C16S);
          break; 
        default:
          TMP117_tmp.setConvTime (C15mS5); 
         break;  
        }

  switch (Averaging)
        {
        case (0):
          TMP117_tmp.setAveraging (NOAVE); 
          break;
        case (1):
          TMP117_tmp.setAveraging (AVE8);         
          break;
        case (2):
          TMP117_tmp.setAveraging (AVE32);
          break;
        case (3):
          TMP117_tmp.setAveraging (AVE64);
          break;
        default:
          TMP117_tmp.setAveraging (NOAVE);
         break;
        }      
        
           TMP117_tmp.update();

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
    
#endif // USES_P136
