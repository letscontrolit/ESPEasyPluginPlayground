//#ifdef PLUGIN_BUILD_TESTING
#ifdef USES_P251

//#######################################################################################################
//#################################### Plugin 251: PZEM004T v30 with modbus mgt##########################
//#######################################################################################################
//

#include <ESPeasySerial.h>
#include <PZEM004Tv30.h>
 
#define PLUGIN_251
#define PLUGIN_ID_251        251
#define PLUGIN_251_DEBUG     true   //activate extra log info in the debug
#define PLUGIN_NAME_251       "PZEM-004Tv30-Multiple"

#define P251_PZEM_mode       PCONFIG(1) //0=read value ; 1=reset energy; 2=programm address
#define P251_PZEM_ADDR       PCONFIG(2)

#define P251_QUERY1          PCONFIG(3)
#define P251_QUERY2          PCONFIG(4)
#define P251_QUERY3          PCONFIG(5)
#define P251_QUERY4          PCONFIG(6)
#define P251_PZEM_FIRST      PCONFIG(7)
#define P251_PZEM_ATTEMPT    PCONFIG(8)

#define P251_PZEM_mode_DFLT  0  // Read value
#define P251_QUERY1_DFLT     0  // Voltage (V)
#define P251_QUERY2_DFLT     1  // Current (A)
#define P251_QUERY3_DFLT     2  // Power (W)
#define P251_QUERY4_DFLT     3  // Energy (WH)
#define P251_NR_OUTPUT_VALUES   4
#define P251_NR_OUTPUT_OPTIONS  6
#define P251_QUERY1_CONFIG_POS  3

#define P251_PZEM_MAX_ATTEMPT      3  // Number of tentative before declaring NAN value

PZEM004Tv30 *P251_PZEM_sensor= nullptr;

boolean Plugin_251_init = false;
uint8_t P251_PZEM_ADDR_SET = 0; // Flag for status of programmation/Energy reset: 0=Reading / 1=Prog confirmed / 3=Prog done / 4=Reset energy done

boolean Plugin_251(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_251;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_251);
        success = true;
        break;
      }

      case PLUGIN_GET_DEVICEVALUENAMES:
      {
        for (byte i = 0; i < VARS_PER_TASK; ++i) {
          if ( i < P251_NR_OUTPUT_VALUES) {
            byte choice = PCONFIG(i + P251_QUERY1_CONFIG_POS);
            safe_strncpy(
              ExtraTaskSettings.TaskDeviceValueNames[i],
              p251_getQueryString(choice),
              sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
          } else {
            ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
          }
        }
        break;
      }

    case PLUGIN_SET_DEFAULTS:
      {
        // Load some defaults
        P251_PZEM_mode = P251_PZEM_mode_DFLT;
        P251_QUERY1 = P251_QUERY1_DFLT;
        P251_QUERY2 = P251_QUERY2_DFLT;
        P251_QUERY3 = P251_QUERY3_DFLT;
        P251_QUERY4 = P251_QUERY4_DFLT;

        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        serialHelper_getGpioNames(event);
        //event->String3 = formatGpioName_output(F("Reset"));
        break;
      }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
      {
        string += serialHelper_getSerialTypeLabel(event);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD: {
      if (P251_PZEM_sensor==nullptr) P251_PZEM_FIRST = event->TaskIndex;  //To detect if first PZEM or not
      if (P251_PZEM_FIRST == event->TaskIndex)  //If first PZEM, serial config available
      {
        serialHelper_webformLoad(event);
        addHtml(F("<br><B>This PZEM is the first. Its configuration of serial Pins will affect next PZEM. </B>"));
        addHtml(F("<span style=\"color:red\"> <br><B>If several PZEMs foreseen, don't use HW serial (or invert Tx and Rx to configure as SW serial).</B></span>"));
        String options_model[3] = {F("Read_value"), F("Reset_Energy"),F("Program_adress")};
        addFormSelector(F("PZEM Mode"), F("P251_PZEM_mode"), 3, options_model, NULL, P251_PZEM_mode);
        
        if (P251_PZEM_mode==2)
        {
          addHtml(F("<span style=\"color:red\"> <br>When programming an address, only one PZEMv30 must be connected. Otherwise, all connected PZEMv30s will get the same address, which would cause a conflict during reading.</span>"));
          String options_confirm[2] = {F("NO"), F("YES")};
          addFormSelector(F("Confirm address programming ?"), F("P251_PZEM_addr_set"), 2, options_confirm, NULL, P251_PZEM_ADDR_SET);
          addFormNumericBox(F("Address of PZEM"), F("P251_PZEM_addr"), (P251_PZEM_ADDR<1)?1:P251_PZEM_ADDR, 1, 247);
          addHtml(F("Select the address to set PZEM. Programming address 0 is forbidden."));
        }      
        else 
        {
          addFormNumericBox(F("Address of PZEM"), F("P251_PZEM_addr"), P251_PZEM_ADDR, 0, 247);
          addHtml(F("  Address 0 allows to communicate with any <B>single</B> PZEMv30 whatever its address"));
        }

        if (P251_PZEM_ADDR_SET==3)  //If address programming done 
        {
          addHtml(F("<span style=\"color:green\"> <br><B>Address programming done ! </B></span>"));
          P251_PZEM_ADDR_SET=0; //Reset programming confirmation
        }
      }
      else
      {
        addHtml(F("Tx Pin and Rx Pin have no effect on the configuration as this PZEM is not the main configured."));
        String options_model[2] = {F("Read_value"), F("Reset_Energy")};
        addFormSelector(F("PZEM Mode"), F("P251_PZEM_mode"), 2, options_model, NULL, P251_PZEM_mode);
        addFormNumericBox(F("Address of PZEM"), F("P251_PZEM_addr"), P251_PZEM_ADDR, 0, 247);
        addHtml(F("  Address 0 allows to communicate with any <B>single</B> PZEMv30 whatever its address"));
      }

      addHtml(F("<br><br> Reset energy can be done also by: http://*espeasyip*/control?cmd=resetenergy,*PZEM address*"));

      if (P251_PZEM_ADDR_SET==4)
      {
        addHtml(F("<span style=\"color:blue\"> <br><B>Energy reset on current PZEM ! </B></span>"));
        P251_PZEM_ADDR_SET=0; //Reset programming confirmation
      }

      // To select the data in the 4 fields. In a separate scope to free memory of String array as soon as possible

      sensorTypeHelper_webformLoad_header();
      String options[P251_NR_OUTPUT_OPTIONS];
      for (uint8_t i = 0; i < P251_NR_OUTPUT_OPTIONS; ++i) {
        options[i] = p251_getQueryString(i);
      }
      for (byte i = 0; i < P251_NR_OUTPUT_VALUES; ++i) {
        const byte pconfigIndex = i + P251_QUERY1_CONFIG_POS;
        sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P251_NR_OUTPUT_OPTIONS, options);
      }

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      serialHelper_webformSave(event);
      // Save output selector parameters.
      for (byte i = 0; i < P251_NR_OUTPUT_VALUES; ++i) {
        const byte pconfigIndex = i + P251_QUERY1_CONFIG_POS;
        const byte choice = PCONFIG(pconfigIndex);
        sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, p251_getQueryString(choice));
      }
      P251_PZEM_mode = getFormItemInt(F("P251_PZEM_mode"));
      P251_PZEM_ADDR = getFormItemInt(F("P251_PZEM_addr"));
      P251_PZEM_ADDR_SET = getFormItemInt(F("P251_PZEM_addr_set")); 
      Plugin_251_init = false; // Force device setup next time
      success = true;
      break;
    }

    case PLUGIN_INIT:
      {
        if (P251_PZEM_FIRST == event->TaskIndex)  //If first PZEM, serial config available
      {
        int rxPin = CONFIG_PIN1;
        int txPin = CONFIG_PIN2;

        if (P251_PZEM_sensor != nullptr) {
          // Regardless the set pins, the software serial must be deleted.
          delete P251_PZEM_sensor;
          P251_PZEM_sensor = nullptr;
        }

        // Hardware serial is RX on 3 and TX on 1        
        P251_PZEM_sensor = new PZEM004Tv30(rxPin, txPin);

        // Sequence for changing PZEM address
        if (P251_PZEM_ADDR_SET == 1)    //if address programming confirmed
        {
          P251_PZEM_sensor->setAddress(P251_PZEM_ADDR);
          P251_PZEM_mode = 0;       // Back to read mode
          P251_PZEM_ADDR_SET = 3;   // Address programmed
        }
      }
        P251_PZEM_sensor->init(P251_PZEM_ADDR);

        // Sequence for reseting PZEM energy
        if (P251_PZEM_mode == 1)
        {
          P251_PZEM_sensor->resetEnergy();
          P251_PZEM_mode = 0;       // Back to read mode
          P251_PZEM_ADDR_SET = 4;   // Energy reset done
        }

        Plugin_251_init = true;
        success = true;
        break;
      }

    case PLUGIN_EXIT:
      {
        if (P251_PZEM_FIRST == event->TaskIndex)  //If first PZEM, serial config available
        {
          if (P251_PZEM_sensor)
          {
            delete P251_PZEM_sensor;
            P251_PZEM_sensor=nullptr;
          }
        }
        break;
      }

    case PLUGIN_READ:
      {
        if (Plugin_251_init && P251_PZEM_mode==0 )    //Read sensor
        {
          // When new data is available, return true
          P251_PZEM_sensor->init(P251_PZEM_ADDR);

          float PZEM[6];  
          PZEM[0] = P251_PZEM_sensor->voltage();
          PZEM[1] = P251_PZEM_sensor->current();
          PZEM[2] = P251_PZEM_sensor->power();
          PZEM[3] = P251_PZEM_sensor->energy();
          PZEM[4] = P251_PZEM_sensor->pf();
          PZEM[5] = P251_PZEM_sensor->frequency();

          for (byte i=0;i<6;i++)    // Check each PZEM field
          {
            if (PZEM[i]!=PZEM[i])   // Check if NAN
            {
              P251_PZEM_ATTEMPT==P251_PZEM_MAX_ATTEMPT? P251_PZEM_ATTEMPT=0:P251_PZEM_ATTEMPT++;
              break;                // if one is Not A Number, break
            }
            P251_PZEM_ATTEMPT=0;
          }

          if (P251_PZEM_ATTEMPT==0)
          { 
            UserVar[event->BaseVarIndex]     = PZEM[P251_QUERY1];
            UserVar[event->BaseVarIndex + 1] = PZEM[P251_QUERY2];
            UserVar[event->BaseVarIndex + 2] = PZEM[P251_QUERY3];
            UserVar[event->BaseVarIndex + 3] = PZEM[P251_QUERY4];
            sendData(event);   //To send externally from the pluggin (to controller or to rules trigger)
          }
          success = true;
        }
        break;
      }

     case PLUGIN_WRITE:
    {
      if (Plugin_251_init)
      {
        String command = parseString(string, 1);
        if (command == F("resetenergy") && (P251_PZEM_FIRST == event->TaskIndex))
        {
          if ((event->Par1 >= 0) && (event->Par1 <= 247))
          {
            P251_PZEM_sensor->init(event->Par1);
            P251_PZEM_sensor->resetEnergy();
            success = true;
          }
        }
      }

       break;
    }
  }
  return success;
}

String p251_getQueryString(byte query) {
  switch(query)
  {
    case 0: return F("Voltage_(V)");
    case 1: return F("Current_(A)");
    case 2: return F("Power_(W)");
    case 3: return F("Energy_(WH)");
    case 4: return F("Power_Factor_(cos-phi)");
    case 5: return F("Frequency (Hz)");
  }
  return "";
}

#endif // USES_P251
