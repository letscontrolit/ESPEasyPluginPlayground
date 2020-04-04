#ifdef USES_P127
//#######################################################################################################
//#################################### Plugin 127: Teleinfo #############################################
//#######################################################################################################
// march 2020 : gmella rewrite macgyver67 version to use linky's STANDARD mode
//                - replace its own teleinfo code using LibTeleinfo
//                - enable use of any controllers instead of hardcoded jeedom using two values
// april 2020 : gmella 
//                - handle basic STANDARD or HISTORIQUE mode (and associated bauds,labels to read)
//                  https://www.enedis.fr/sites/default/files/Enedis-NOI-CPT_54E.pdf
//                - test works on weemos mini D1 and Lolin NodeMcu V3 using HW serial 0 (serial1 fails)
//                  using domoticz controller (published each 20s) + oled display.
//              TODO 
//                - keep TInfo object shared accross multiple device to help the read of other labels
//                  ( such demo were shown using multiples PZEM004 )
//                - prefer previous embedded teleinfo code to follow plugin guidelines 
//                  original hallard's lib was heavily forked but suffer from feedback and PR
//                  test using https://github.com/marco402/LibTeleinfo/tree/rewrite-Wifinfo/examples/Wifinfo
//                             ( with "mySyslog.h" and "Wifinfo.h" commented in LibTeleinfo.h )
//
// 

#define PLUGIN_127
#define PLUGIN_ID_127 127
#define PLUGIN_NAME_127 "Energy - Teleinfo Power Energy [TESTING]"

// Default labels for both TIC modes
#define PLUGIN_VALUENAME1_127 "PAPP"
#define PLUGIN_VALUENAME2_127 "HCHP"  // or HCHC 
#define PLUGIN_VALUENAME1_STD_127 "SINST"
#define PLUGIN_VALUENAME2_STD_127 "EAST"  // and "HCHP" or HCHC 

#include <SoftwareSerial.h>
#include <LibTeleinfo.h>

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
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
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
        // Init teleinfo SDT mode
        tinfo.init(false);

        // Init Serial
        const int16_t serial_rx = CONFIG_PIN1;
        const int16_t serial_tx = CONFIG_PIN2;        
        P127_easySerial = new ESPeasySerial(serial_rx, serial_tx);

        String log = F("P127 : Init OK ");
        if (PCONFIG(0)){
          P127_easySerial->begin((unsigned long)9600, SERIAL_7E1, SERIAL_RX_ONLY);
          log += F("9600_7E1 RX=");
        }else{
          P127_easySerial->begin((unsigned long)1200, SERIAL_7E1, SERIAL_RX_ONLY);
          log += F("1200_7E1 RX=");
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

        event->sensorType = SENSOR_TYPE_DUAL;
        success = true;
        int value;
        char cvalue[TAILLE_MAX_VALUE]; 
        char *ret;

        String log = F("P127 : ");
        
        for (byte x = 0; x < 2; x++)
        {
          log += ExtraTaskSettings.TaskDeviceValueNames[x];          
          log += F("=");          
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

        addFormCheckBox(F("Use STANDARD mode"), F("p127_standard_mode"), PCONFIG(0));
        
        success = true;
        break;
      }
    case PLUGIN_WEBFORM_SAVE:
      {
        serialHelper_webformSave(event);

        PCONFIG(0) = isFormItemChecked(F("p127_standard_mode"));

        // Set default label for associated mode keeping user input if any
        if (PCONFIG(0)){
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[0],PSTR( PLUGIN_VALUENAME1_127))==0 ){
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_STD_127));
          }
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_127))==0 ){
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_STD_127));        
          }
        }else{
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[0],PSTR( PLUGIN_VALUENAME1_STD_127))==0 ){
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_127));
          }
          if( strcmp_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_STD_127))==0 ){
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_127));
          }
        }       

        success = true;
        break;
      }
  }
  return success;
}

#endif // USES_P127
