//#######################################################################################################
//################################ Plugin 203: Pet feeder     ###########################################
//#######################################################################################################
//############################### Plugin for ESP Easy by DatuX            ###############################
//################################### http://www.datux.nl  ##############################################
//#######################################################################################################



#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_203
#define PLUGIN_ID_203         203
#define PLUGIN_NAME_203       "Automated pet feeder [TESTING]"

#ifndef CONFIG
#define CONFIG(n) (Settings.TaskDevicePluginConfig[event->TaskIndex][n])
#endif





boolean Plugin_203(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number = PLUGIN_ID_203;
      Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
      Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
      Device[deviceCount].Ports = 0;
      Device[deviceCount].PullUpOption = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption = false;
      Device[deviceCount].ValueCount = 1;
      Device[deviceCount].SendDataOption = true;
      Device[deviceCount].TimerOption = false;
      Device[deviceCount].GlobalSyncOption = false;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_203);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {

      addFormNumericBox(string, F("Rotate forward time"), F("forward"), CONFIG(0), 0, 60000);
      addUnit(string, F("ms"));
      addFormNumericBox(string, F("Rotate reverse time"), F("reverse"), CONFIG(1), 0, 60000);
      addUnit(string, F("ms"));



      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      CONFIG(0) = getFormItemInt(F("forward"));
      CONFIG(1) = getFormItemInt(F("reverse"));

      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      pinMode(Settings.TaskDevicePin1[event->TaskIndex],OUTPUT);
      pinMode(Settings.TaskDevicePin2[event->TaskIndex],OUTPUT);
      pinMode(Settings.TaskDevicePin3[event->TaskIndex],OUTPUT);
      digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW); //disable
      success = true;
      break;
    }



    case PLUGIN_WRITE:
    {

      String command = parseString(string, 1);

      if (command == F("feed"))
      {
        for(int i=0; i<event->Par1; i++)
        {
          //forward
          digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH); //enable
          digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);
          digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], LOW);
          delay(CONFIG(0));

          //pause
          digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW); //disable
          delay(100);

          //reverse
          digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH); //enable
          digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);
          digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], HIGH);
          delay(CONFIG(1));

          //pause
          digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW); //disable
          delay(100);

        }


      }


      success=true;
      break;
    }

  }
  return success;
}

#endif
