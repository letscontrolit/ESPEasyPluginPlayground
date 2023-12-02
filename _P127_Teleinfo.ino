#ifdef USES_P127
//#######################################################################################################
//#################################### Plugin 127: Teleinfo #############################################
//#######################################################################################################
// 11/2023 : update code for working with last ESPeasy releases
// march 2020 : gmella rewrite macgyver67 
//                - replace its own teleinfo code using LibTeleinfo
//                - enable use of any controllers instead of hardcoded jeedom using two values
// april 2020 : gmella 
//                - support STANDARD or HISTORIQUE mode 
//                  https://www.enedis.fr/sites/default/files/Enedis-NOI-CPT_54E.pdf Enedis-NOI-CPT_54E
//                - test works on weemos mini D1 and Lolin NodeMcu V3 using HW serial 0 (serial1 fails)
//                  using domoticz controller (published each 20s) + oled display.
//                  on LINKY and A14C4 counters
//              TODO 
//                - prefer previous macgyver67 embedded teleinfo code to follow plugin guidelines over
//                  modified LibTeleinfo library (see P127_LibTeleinfo_Library directory)
//                - keep TInfo object shared accross multiple devices to help the read of other labels
//                  ( such concept were shown using multiples PZEM004 )
// 

#define PLUGIN_127
#define PLUGIN_ID_127 127
#define PLUGIN_NAME_127 "Energy - Teleinfo Power Energy [TESTING]"

// Default labels for both TIC modes
#define PLUGIN_VALUENAME1_127 "SINST"
#define PLUGIN_VALUENAME2_127 "EAST"
#define PLUGIN_VALUENAME1_HISTO_127 "PAPP"
#define PLUGIN_VALUENAME2_HISTO_127 "BASE"

#include <SoftwareSerial.h>
#include <ESPeasySerial.h>
#include <ESPEasySerialPort.h>
#include <LibTeleinfo.h> // find modified copy info in P127_LibTeleinfo_Library/Readme.md

TInfo          tinfo;                     // Teleinfo object 
ESPeasySerial *P127_easySerial = nullptr; //Associated serial line

boolean Plugin_127_init = false;

boolean Plugin_127(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_127;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = Sensor_VType::SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        Device[deviceCount].PluginStats = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_127);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_127));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_127));
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        serialHelper_getGpioNames(event, false, false); // TX optional
        break;
      }

    case PLUGIN_INIT:
      {
        // Init Serial
        const int16_t serial_rx = CONFIG_PIN1;
        const int16_t serial_tx = CONFIG_PIN2;        
        P127_easySerial = new ESPeasySerial(ESPEasySerialPort::not_set, serial_rx, serial_tx);

        String log = F("P127 : Init ");
        if (PCONFIG(0)){
          // Init teleinfo and serial for HISTORIQUE mode
          tinfo.init(true);          
          P127_easySerial->begin((unsigned long)1200, SERIAL_7E1, SERIAL_RX_ONLY);
          log += F("HISTORIQUE mode 1200_7E1 RX=");          
        }else{
          // Init teleinfo and serial for STANDARD mode
          tinfo.init(false);          
          P127_easySerial->begin((unsigned long)9600, SERIAL_7E1, SERIAL_RX_ONLY);
          log += F("STANDARD mode 9600_7E1 RX=");          
        }
        
        log += serial_rx;        
        log += F(" reading ");       
        log += ExtraTaskSettings.TaskDeviceValueNames[0];
        log += F(",");       
        log += ExtraTaskSettings.TaskDeviceValueNames[1];
        addLog(LOG_LEVEL_INFO, log);
        
        Plugin_127_init = true;
        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        if ( ! Plugin_127_init ) break ;

        event->sensorType = Sensor_VType::SENSOR_TYPE_DUAL;
        success = true;
        int value;
        char cvalue[TAILLE_MAX_VALUE]; 
        char *ret;

        String log = F("P127 : ");
        
        for (byte x = 0; x < 2; x++)
        {
          log += ExtraTaskSettings.TaskDeviceValueNames[x];          
          log += F("=");          
          ZERO_FILL(cvalue);
          ret = tinfo.valueGet(ExtraTaskSettings.TaskDeviceValueNames[x], cvalue);                          
          if (ret != NULL){
            value=atoi(cvalue);       
            UserVar[event->BaseVarIndex+x] = (float)value;                    
            log += value;                             
          }else{
            log += F("NC");
            success = false;
          }                
          log += F(" ");  
        }
        addLog(LOG_LEVEL_INFO, log);        
       
        break;
      }      

    case PLUGIN_TEN_PER_SECOND:
      {
        if ( ! Plugin_127_init ) break ;
        unsigned long timeout = millis() + 10;                      
        while (P127_easySerial->available() &&  millis() < timeout ) {            
          tinfo.process(P127_easySerial->read());   
        }
        success = true; 
        break;       
      }
     
    case PLUGIN_WEBFORM_SHOW_CONFIG:
      {
        string += serialHelper_getSerialTypeLabel(event);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        serialHelper_webformLoad(event);

        addFormCheckBox(F("Use HISTORIQUE mode"), F("p127_historique_mode"), PCONFIG(0));
        
        success = true;
        break;
      }
    case PLUGIN_WEBFORM_SAVE:
      {
        serialHelper_webformSave(event);

        PCONFIG(0) = isFormItemChecked(F("p127_historique_mode"));

        // Set default label for associated mode keeping user input if any
        if (PCONFIG(0)){
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[0],PSTR( PLUGIN_VALUENAME1_127))==0 )
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_HISTO_127));
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_127))==0 )
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_HISTO_127));        
        }else{
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[0],PSTR( PLUGIN_VALUENAME1_HISTO_127))==0 )
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_127));
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_HISTO_127))==0 )
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_127));
        }       

        success = true;
        break;
      }
  }
  return success;
}

#endif // USES_P127
