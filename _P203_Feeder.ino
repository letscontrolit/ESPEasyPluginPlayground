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



//to not scare the cat of sudden motor movements
//attack and sustain in mS
void ramped_pulse(byte pin, unsigned long attack, unsigned long sustain)
{

    //attack
    unsigned long start_time=millis();
    while (millis()-start_time < attack)
    {
      analogWrite(pin, ((millis()-start_time) * PWMRANGE /attack ) );
      yield();
    }

    //sustain
    analogWrite(pin, PWMRANGE);
    // digitalWrite(pin, HIGH);
    delay(sustain);

    //releaase
    start_time=millis();
    while (millis()-start_time < attack)
    {
      analogWrite(pin, PWMRANGE-(((millis()-start_time) * PWMRANGE / attack) ));
      yield();
    }

    //make sure its really off
    analogWrite(pin, 0);
    // digitalWrite(pin, LOW);
}

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

      addFormNumericBox(F("Rotate forward time"), F("forward"), CONFIG(0), 0, 60000);
      addUnit(F("ms"));
      addFormNumericBox(F("Rotate reverse time"), F("reverse"), CONFIG(1), 0, 60000);
      addUnit(F("ms"));



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
      // analogWriteFreq(30000);
      digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW); //disable
      success = true;
      break;
    }



    case PLUGIN_WRITE:
    {

      String command = parseString(string, 1);

      if (command == F("feed"))
      {
        // for(int i=0; i<event->Par1; i++)
        {
          //forward
          // digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH); //enable
          digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);
          digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], LOW);

          ramped_pulse(Settings.TaskDevicePin1[event->TaskIndex], event->Par1, CONFIG(0));
          // delay(CONFIG(0));

          //pause
          // digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW); //disable
          // delay(100);

          //reverse
          // digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], HIGH); //enable
          digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);
          digitalWrite(Settings.TaskDevicePin3[event->TaskIndex], HIGH);
          ramped_pulse(Settings.TaskDevicePin1[event->TaskIndex], 1000, CONFIG(1));
          // delay(CONFIG(1));

          //pause
          // digitalWrite(Settings.TaskDevicePin1[event->TaskIndex], LOW); //disable
          // delay(100);

        }


      }


      success=true;
      break;
    }

  }
  return success;
}

#endif
