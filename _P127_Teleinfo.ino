#ifdef USES_P127
//#######################################################################################################
//#################################### Plugin 127: Teleinfo #############################################
//#######################################################################################################
// march 2020 : gmella rewrite macgyver67 version 
//                - replace its own teleinfo code using LibTeleinfo
//                - create two values to be transmitted by controllers if any

#define PLUGIN_127
#define PLUGIN_ID_127 127
#define PLUGIN_NAME_127 "Energy - Teleinfo Power Energy"
// TODO add switch so the user can choose STANDARD or HISTORIQUE (and associated bauds,labels to read)?

#define PLUGIN_VALUENAME1_127 "SINST" // should be "TBD" for HISTORIQUE
#define PLUGIN_VALUENAME2_127 "EAST"  // and "TBD" 

#include <SoftwareSerial.h>
#include <LibTeleinfo.h>

TInfo          tinfo; // Teleinfo object (could be a singleton so we can read other label on the same instance (serialline) )
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
        P127_easySerial->begin((unsigned long)9600, SERIAL_7E1, SERIAL_RX_ONLY);

        Plugin_127_init = true;
        
        String log = F("P127 : Init OK  9600_7E1 RX:");
        log += serial_rx;        
        log += F(" reading ");       
        log += ExtraTaskSettings.TaskDeviceValueNames[0];
        log += F(",");       
        log += ExtraTaskSettings.TaskDeviceValueNames[1];
        addLog(LOG_LEVEL_INFO, log);
        
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
        success = true;
        break;
      }
    case PLUGIN_WEBFORM_SAVE:
      {
        serialHelper_webformSave(event);
        success = true;
        break;
      }
  }
  return success;
}

#endif // USES_P127
