//#######################################################################################################
//#################################### Plugin 180: Analog ###############################################
//#######################################################################################################

#define PLUGIN_180
#define PLUGIN_ID_180         180
#define PLUGIN_NAME_180       "Mux Analog input - 74151/74152/74153 [TESTING]"
#define PLUGIN_VALUENAME1_180 "Analog"
boolean Plugin_180(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_180;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 8;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_180);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_180));
        break;
      }
    case PLUGIN_INIT:
      {
        int availablePorts=1;
        if (Settings.TaskDevicePin1[event->TaskIndex] != -1)
        {
          pinMode(Settings.TaskDevicePin1[event->TaskIndex], OUTPUT);
          availablePorts*=2;
        }
        if (Settings.TaskDevicePin2[event->TaskIndex] != -1)
        {
          pinMode(Settings.TaskDevicePin2[event->TaskIndex], OUTPUT);
          availablePorts*=2;
        }
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1)
        {
          pinMode(Settings.TaskDevicePin3[event->TaskIndex], OUTPUT);
          availablePorts*=2;
        }
        String log = F("Mux available ports  : ");
        log += availablePorts;
        addLog(LOG_LEVEL_INFO,log);
        success = true;
        break;
    }
    case PLUGIN_READ:
      {
        String log = F("ADC  : Analog port ");
        log += Settings.TaskDevicePort[event->TaskIndex];
        log += F(" mux address: ");
        switch( Settings.TaskDevicePort[event->TaskIndex] )   
        {  
            case 1:  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], HIGH);
              log += " 001 "; 
              break;
            case 2:  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], LOW);
              log += " 010 "; 
              break;
            case 3:  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], HIGH);
              log += " 011 "; 
              break;
            case 4:  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], LOW);
              log += " 100 "; 
              break;
            case 5:  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], HIGH);
              log += " 101 "; 
              break;
            case 6:  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], LOW);
              log += " 110 "; 
              break;
            case 7:  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], HIGH);
              log += " 111 "; 
              break;
            default :  
              digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);
              digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], LOW);
              log += " 000 "; 
        }  

        UserVar[event->BaseVarIndex] = analogRead(A0);

        log += F("value: ");
        log += UserVar[event->BaseVarIndex];
        addLog(LOG_LEVEL_INFO,log);
        success = true;
        break;
      }
  }
  return success;
}