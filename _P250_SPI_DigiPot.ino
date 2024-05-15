//#######################################################################################################
//################################### Plugin 250: SPI_DigiPot ###########################################
//############################# For MCP 4151 add only one instance ######################################
//############################# For MCP 42010 add two instances, ... ####################################
//#######################################################################################################
// written by antibill

// Usage:
// (1): Set value to potentiometer (http://xx.xx.xx.xx/control?cmd=digipot,0,255)
// (2): Set value to potentiometer (http://xx.xx.xx.xx/control?cmd=digipot,1,0)

static uint16_t Plugin_250_PotDest[2] = {0,0}; // 2 is the number of max digipot. if 4, then {0,0,0,0}. Adapt also uint16t.
uint8_t Plugin_250_pot_number = 0; // initial value

#define PLUGIN_250
#define PLUGIN_ID_250         250
#define PLUGIN_NAME_250       "SPI_DigiPot"
#define PLUGIN_VALUENAME1_250 "DigiPot"




boolean Plugin_250(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {


        Device[++deviceCount].Number = PLUGIN_ID_250;
        Device[deviceCount].Type = DEVICE_TYPE_SPI;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_SINGLE;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_250);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_250));
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
        addFormNumericBox(F("DigiPot Number"), F("p250_pot_number"), PCONFIG(0), 0, 255);
        addFormNote(F("<b>1st DigiPot is 0, 2nd : 1, ... "));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG(0) = getFormItemInt(F("p250_pot_number"));
        Plugin_250_pot_number = PCONFIG(0);
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
      uint8_t CS_pin_no = get_SPI_CS_Pin(event);
      Plugin_250_pot_number = PCONFIG(0);
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
        uint8_t pot_number = PCONFIG(0);
        uint8_t CS_pin_no = get_SPI_CS_Pin(event);
        String tmpString  = string;
        int argIndex = tmpString.indexOf(',');
        if (argIndex)
          tmpString = tmpString.substring(0, argIndex);

        if (tmpString.equalsIgnoreCase(F("digipot")))
        {
          uint8_t pot;
          uint16_t value;
          pot = event->Par1;
          value = event->Par2;
          if (pot_number == pot)
          {
            if (value> 255)
              {
                value = 255;
              }
            handle_SPI_CS_Pin(CS_pin_no, LOW);
            SPI.transfer(((pot +1) & B11) | B00010000);
            SPI.transfer(value);
            handle_SPI_CS_Pin(CS_pin_no, HIGH);
            Plugin_250_PotDest[pot] = value;
            UserVar[event->BaseVarIndex + 0 ] = Plugin_250_PotDest[pot] ;
            success = true;
          }
        }
        break;
      }

    case PLUGIN_READ:
      {
        int8_t pot_number = PCONFIG(0);
        UserVar[event->BaseVarIndex + 0] = Plugin_250_PotDest[pot_number] ;
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
