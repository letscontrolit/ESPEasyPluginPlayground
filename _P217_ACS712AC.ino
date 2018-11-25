
#ifdef PLUGIN_BUILD_DEV

  //Plugin to measure AC current using a ACS712 module. Very much a work in progress.
  //For my lab, i have used R1=1200 and R2=2000
  #define PLUGIN_217
  #define PLUGIN_ID_217     217               //plugin id
  #define PLUGIN_NAME_217   "Energy (AC) - ACS712 [DEVELOPMENT]"     //"Plugin Name" is what will be dislpayed in the selection list
  #define PLUGIN_VALUENAME1_217 "A"     //variable output of the plugin. The label is in quotation marks
  #define PLUGIN_217_DEBUG  true             //set to true for extra log info in the debug
  #define PLUGIN_217_MODEL_5A 185
  #define PLUGIN_217_MODEL_20A 100
  #define PLUGIN_217_MODEL_30A 66

  boolean Plugin_217(byte function, struct EventStruct *event, String& string)
  {
    boolean success = false;
    switch (function)
    {
      case PLUGIN_DEVICE_ADD:
      {

          //This case defines the device characteristics, edit appropriately

          Device[++deviceCount].Number = PLUGIN_ID_217;
          Device[deviceCount].Type = DEVICE_TYPE_ANALOG;  //how the device is connected
          Device[deviceCount].VType = SENSOR_TYPE_SINGLE; //type of value the plugin will return, used only for Domoticz
          Device[deviceCount].Ports = 0;
          Device[deviceCount].PullUpOption = false;
          Device[deviceCount].InverseLogicOption = false;
          Device[deviceCount].FormulaOption = false;
          Device[deviceCount].ValueCount = 1;             //number of output variables. The value should match the number of keys PLUGIN_VALUENAME1_xxx
          Device[deviceCount].SendDataOption = true;
          Device[deviceCount].TimerOption = true;
          Device[deviceCount].TimerOptional = false;
          Device[deviceCount].GlobalSyncOption = true;
          Device[deviceCount].DecimalsOnly = true;
          break;
      }
      case PLUGIN_GET_DEVICENAME:
      {
        //return the device name
        string = F(PLUGIN_NAME_217);
        break;
      }
      case PLUGIN_GET_DEVICEVALUENAMES:
      {
        //called when the user opens the module configuration page
        //it allows to add a new row for each output variable of the plugin
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_217));
        break;
      }
      case PLUGIN_WEBFORM_LOAD:
      {
        String model[3];
        model[0] = F("ACS712 5A");
        model[1] = F("ACS712 20A");
        model[2] = F("ACS712 30A");
        byte modelchoice = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        int modelValues[3] = { PLUGIN_217_MODEL_5A, PLUGIN_217_MODEL_20A, PLUGIN_217_MODEL_30A };
        addFormSubHeader(F("Model selection"));
        addFormSelector(F("ACS712 model"), F("plugin_217_model"), 3, model, modelValues, modelchoice);
        addFormSubHeader(F("Sampling period"));
        addFormNumericBox(F("Sampling period in ms"), F("plugin_217_sampling_period"), ExtraTaskSettings.TaskDevicePluginConfigLong[2]);
        addFormSubHeader(F("Voltage divider values"));
        addFormNumericBox(F("R1 Value, Ohms"), F("plugin_217_r1"), ExtraTaskSettings.TaskDevicePluginConfigLong[0]);
        addFormNumericBox(F("R2 Value, Ohms"), F("plugin_217_r2"), ExtraTaskSettings.TaskDevicePluginConfigLong[1]);
        addFormSubHeader(F("Result offset"));

        success = true;
        break;
      }
      case PLUGIN_WEBFORM_SAVE:
      {
        //this case defines the code to be executed when the form is submitted
        //the plugin settings should be saved to Settings.TaskDevicePluginConfig[event->TaskIndex][x]
        //ping configuration should be read from Settings.TaskDevicePin1[event->TaskIndex] and stored
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_217_model"));
        ExtraTaskSettings.TaskDevicePluginConfigLong[0] = getFormItemInt(F("plugin_217_r1"));
        ExtraTaskSettings.TaskDevicePluginConfigLong[1] = getFormItemInt(F("plugin_217_r2"));
        ExtraTaskSettings.TaskDevicePluginConfigLong[2] = getFormItemInt(F("plugin_217_sampling_period"));
        //after the form has been saved successfuly, set success and break
        success = true;
        break;

      }
      case PLUGIN_INIT:
      {
        //this case defines code to be executed when the plugin is initialised

        //after the plugin has been initialised successfuly, set success and break
        success = true;
        break;

      }

      case PLUGIN_READ:
      {

        int mVperAmp = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        float R1 = ExtraTaskSettings.TaskDevicePluginConfigLong[0];
        float R2 = ExtraTaskSettings.TaskDevicePluginConfigLong[1];
        double Voltage = 0;
        double VRMS = 0;
        double AmpsRMS = 0;
        double VoltageSensor = 0;
        Voltage = p217_get_VPP();


        VoltageSensor = (Voltage*(R2/R1));
        VRMS = (VoltageSensor/2.0) *0.707;
        AmpsRMS = (VRMS * 1000)/mVperAmp;
        UserVar[event->BaseVarIndex] = (float)AmpsRMS;
        #ifdef PLUGIN_217_DEBUG
          String log = "";
          log = "Voltage:";
          log += String(Voltage);
          log += ", R1:";
          log += String(R1);
          log += ", R2:";
          log += String(R2);
          log += " VoltageSensor:";
          log += String(VoltageSensor);
          log += ", AmpsRMS:";
          log += String(AmpsRMS);
          addLog(LOG_LEVEL_INFO,String(log));
        #endif
        success = true;
        break;

      }
      case PLUGIN_EXIT:
    	{
    	  //perform cleanup tasks here. For example, free memory

    	  break;

    	}

      case PLUGIN_ONCE_A_SECOND:
      {
        //code to be executed once a second. Tasks which do not require fast response can be added here
        success = true;
      }
    }
    return success;
  }

float p217_get_VPP()
  {
    long timePassedSince(unsigned long timestamp);
    boolean timeOutReached(unsigned long timer);
    float sampling_period = ExtraTaskSettings.TaskDevicePluginConfigLong[2];
    const int sensorIn = A0;
    float result;
    int readValue;             //value read from the sensor
    int maxValue = 0;          // store max value here
    int minValue = 1024;          // store min value here
    uint32_t start_time = millis();
    uint32_t nice_time = millis();
   
   while(timePassedSince(start_time) < sampling_period) //sample the period specified
   {
       readValue = analogRead(sensorIn);
       //After a friendly tip in my pull request it was suggested that one should
       //call delay(0) periodically to increase stability. So, lets do that every 20ms
       if (timePassedSince(nice_time) > 20 ){
         delay(0);
         nice_time = millis();
       }

       // see if you have a new maxValue
       if (readValue > maxValue)
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue)
       {
           /*record the maximum sensor value*/
           minValue = readValue;
       }
   }

   // Subtract min from max
   result = ((maxValue - minValue) * 5.0)/1024.0;

   return result;

  }
#endif
