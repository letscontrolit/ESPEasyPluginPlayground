//#######################################################################################################
//#################################### Plugin 152: MCP42010 #############################################
//################################## I use GPIO 12 / 14 / 15 ############################################
//#######################################################################################################
// written by antibill

// Usage:
// (1): Set value to potentiometer (http://xx.xx.xx.xx/control?cmd=digipot,0,255)
// (2): Set value to potentiometer (http://xx.xx.xx.xx/control?cmd=digipot,1,0)

static int16_t Plugin_152_PotDest[2] = {0,0};

#define PLUGIN_152
#define PLUGIN_ID_152         152
#define PLUGIN_NAME_152       "MCP42010"
#define PLUGIN_VALUENAME1_152 "DigiPot0"
#define PLUGIN_VALUENAME2_152 "DigiPot1"



boolean Plugin_152(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_152;
        Device[deviceCount].Type = DEVICE_TYPE_SPI;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_DUAL;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_152);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_152));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_152));
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        event->String1 = formatGpioName_output(F("CS"));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        # ifdef ESP8266
          {
            addFormNote(F("<b>1st GPIO</b> = CS (Usable GPIOs : 0, 2, 4, 5, 15)"));
          }
        # endif // ifdef ESP8266
        # ifdef ESP32
          {
            addFormNote(F("<b>1st GPIO</b> = CS (Usable GPIOs : 0, 2, 4, 5, 15..19, 21..23, 25..27, 32, 33)"));
          }
        # endif // ifdef ESP32
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
      uint8_t CS_pin_no = get_SPI_CS_Pin(event);
   
      // set the slaveSelectPin as an output:
      init_SPI_CS_Pin(CS_pin_no);

      // initialize SPI:
      SPI.setHwCs(false);
      SPI.begin();
        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        uint8_t CS_pin_no = get_SPI_CS_Pin(event);
        String tmpString  = string;
        int argIndex = tmpString.indexOf(',');
        if (argIndex)
          tmpString = tmpString.substring(0, argIndex);

        if (tmpString.equalsIgnoreCase(F("digipot")))
        {
          int pot;
          int value;
          pot = event->Par1;
          value = event->Par2;

          
          handle_SPI_CS_Pin(CS_pin_no, LOW);
          SPI.transfer(((pot +1) & B11) | B00010000);
          SPI.transfer(value);
          handle_SPI_CS_Pin(CS_pin_no, HIGH);
          Plugin_152_PotDest[pot] = value;
          UserVar[event->BaseVarIndex + pot] = Plugin_152_PotDest[pot] ;
          UserVar[event->BaseVarIndex + pot] = Plugin_152_PotDest[pot] ;

          success = true;
        }


        break;
      }

    case PLUGIN_READ:
      {

        UserVar[event->BaseVarIndex + 0] = Plugin_152_PotDest[0] ;
        UserVar[event->BaseVarIndex + 1] = Plugin_152_PotDest[1] ;
        success = true;
        break;
      }


  }
  return success;
}
/**************************************************************************/
int get_SPI_CS_Pin(struct EventStruct *event) {  // If no Pin is in Config we use 15 as default -> Hardware Chip Select on ESP8266
  if (CONFIG_PIN1 != 0) {
    return CONFIG_PIN1;
  }
  return 15; 
}

/**************************************************************************/
/*!
    @brief Initializing GPIO as OUTPUT for CS for SPI communication
    @param CS_pin_no the GPIO pin number used as CS
    @returns
    
    Initial Revision - chri.kai.in 2021 
/**************************************************************************/
void init_SPI_CS_Pin (uint8_t CS_pin_no) {
  
      // set the slaveSelectPin as an output:
      pinMode(CS_pin_no, OUTPUT);

}

/**************************************************************************/
/*!
    @brief Handling GPIO as CS for SPI communication
    @param CS_pin_no the GPIO pin number used as CS
    @param l_state the state of the CS pin: "HIGH/LOW" reflecting the physical level
    @returns
    
    Initial Revision - chri.kai.in 2021 
/**************************************************************************/
void handle_SPI_CS_Pin (uint8_t CS_pin_no, bool l_state) {
  
  digitalWrite(CS_pin_no, l_state);
}
