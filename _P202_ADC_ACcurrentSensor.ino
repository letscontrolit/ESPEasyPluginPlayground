//#######################################################################################################
//#################################### Plugin 202: AC current C.T. sensor - mod(c) ######################
//#######################################################################################################

#define PLUGIN_202
#define PLUGIN_ID_202         202
#define PLUGIN_NAME_202       "Analog AC sensor (mod c)"
#define PLUGIN_VALUENAME1_202 "nVPP"
#define PLUGIN_VALUENAME2_202 "ACcurrentRMS"
#define PLUGIN_VALUENAME3_202 "ACwatts"

// float Plugin_202_nVPP[TASKS_MAX];                 // Signal voltage measured across C.T. resistor, converted to float.
float Plugin_202_nVPP;
//float Plugin_202_nCurrThruResistorPP[TASKS_MAX];  // peak to peak current through resistor.
//float Plugin_202_nCurrThruResistorPP;
//float Plugin_202_nCurrThruResistorRMS[TASKS_MAX]; // RMS current through Resistor

//float Plugin_202_nCurrentThruWire[TASKS_MAX];     // Actual RMS current in Wire
float Plugin_202_nCurrentThruWire;
//float Plugin_202_watts[TASKS_MAX];                // watts (VA) assuming constant mains voltage and resistive load.
float Plugin_202_watts;

/*
// Parameters (variables so they can eventually be changed via web interface)

 int plugin_202_CT_ratio[TASKS_MAX];  // turns ratio of Current Transformer
 int plugin_202_Resistor_ohms[TASKS_MAX]; // burden resistor value 200
 int plugin_202_mains_volts[TASKS_MAX]; // assumed to be constant voltage 241
 float plugin_202_current_zero_error[TASKS_MAX]; // 93.0 zero correction in mA (from independent measurement)
*/
boolean Plugin_202(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_202;
        Device[deviceCount].Type = DEVICE_TYPE_ANALOG;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM_BARO;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_202);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_202));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_202));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_202));
        
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      // gets previously saved parameter values into the Device configuration page ... 
      {
        char tmpString[256]; // was 128 - too small?

        sprintf_P(tmpString, PSTR("<TR><TD>CT ratio:<TD><input type='text' name='plugin_202_CT_ratio' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        string += tmpString;

        sprintf_P(tmpString, PSTR("<TR><TD>Resistor ohms :<TD><input type='text' name='plugin_202_Resistor_ohms' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        string += tmpString;

        sprintf_P(tmpString, PSTR("<TR><TD>Mains volts :<TD><input type='text' name='plugin_202_mains_volts' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        string += tmpString;

        sprintf_P(tmpString, PSTR("<TR><TD>Current Zero Error (mA) :<TD><input type='text' name='plugin_202_current_zero_error' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][3]);
        string += tmpString;
                
        success = true;
        break;        
      }

    case PLUGIN_WEBFORM_SAVE:
      // saves the parameter values entered via the Device configuration page...
    
      {

        String plugin1 = WebServer.arg("plugin_202_CT_ratio");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();

        String plugin2 = WebServer.arg("plugin_202_Resistor_ohms");
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();

        String plugin3 = WebServer.arg("plugin_202_mains_volts");
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = plugin3.toInt();

        String plugin4 = WebServer.arg("plugin_202_current_zero_error");
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = plugin4.toInt();
        
        success = true;
        break;   
        
      }


    case PLUGIN_WEBFORM_SHOW_VALUES:
      // This section displays most recent measurred values in the "Devices" table of the web interface....
      
      {
        string += ExtraTaskSettings.TaskDeviceValueNames[0];
        string += F(":");
        string += UserVar[event->BaseVarIndex];
        string += F("<BR>");

        string += ExtraTaskSettings.TaskDeviceValueNames[1];
        string += F(":");
        string += UserVar[event->BaseVarIndex+1];
        string += F("<BR>");

        string += ExtraTaskSettings.TaskDeviceValueNames[2];
        string += F(":");
        string += UserVar[event->BaseVarIndex+2];
        string += F("<BR>");
/*
        string += ExtraTaskSettings.TaskDeviceValueNames[3];
        string += F(":");
        string += plugin_202_current_zero_error[event->TaskIndex];
        string += F("<BR>");
*/                
        success = true;
        break;  
      }

       
      
    case PLUGIN_READ:
        // This section gets a new value from the sensor and does the calculations, displays measurements and sends info to the Log...
      {
        Plugin_202_nVPP = Plugin_202_getVPP(); // Calls method below to sample AC waveform and pick up the peak voltage
                                                // Signal represents an integer between 0 and 1024

           // Convert the peak analog value to a peak voltage across the sensor's burden resistor

            Plugin_202_nVPP *= 3.3;     // NodeMCU works at 3.3v (BUT sensor output is quoted as 1 volt max, so this may change!)
            Plugin_202_nVPP *= 1.056;  // empirical range correction (from comparison with an energy measuring plug device)
            Plugin_202_nVPP /= 1024.0;  // 1024 analog values in range.
 //           (nVPP now contains the peak voltage across the burden resistor)          
        
        Plugin_202_nCurrentThruWire = (Plugin_202_nVPP / 200.0) * 1000.0;      // 200 = Resistor value, 1000 = conversion to mA
        Plugin_202_nCurrentThruWire *= 0.707; // factor for sinewave to convert to RMS (assumes pure resistive load!)

        Plugin_202_nCurrentThruWire *= 1000.0;   // 1000 is CT ratio
        Plugin_202_nCurrentThruWire -= 25.0;  // (subtract zero error in mA)
         if (Plugin_202_nCurrentThruWire < 0.0 ) {
            Plugin_202_nCurrentThruWire = 0.0;    // eliminate negative values      
        }

// Now estimate Power in Watts....
//        Plugin_202_watts = 241.0 * Plugin_202_nCurrentThruWire / 1000.0; // 241 = Plugin_202_mains_volts
          // calculate watts (V*A), assumes constant mains voltage and 100% PF (pure resistive load, and pure sine wave, as approximation!)
          Plugin_202_watts = Plugin_202_nCurrentThruWire * Settings.TaskDevicePluginConfig[event->TaskIndex][2]; // multiply by mains volts
          Plugin_202_watts /= 1000.0; // convert mW to Watts

// Update measurements ...                
        UserVar[event->BaseVarIndex] = (float) Plugin_202_nVPP; // peak to peak signal volts
        UserVar[event->BaseVarIndex+1] = Plugin_202_nCurrentThruWire; // RMS AC current in mA 
        UserVar[event->BaseVarIndex+2] = Plugin_202_watts; // Watts   

// Update the Log...
        String log = F(" nVPP  : ");
        log += UserVar[event->BaseVarIndex];
        log += F(" BaseVar+1 : ");
        log += UserVar[event->BaseVarIndex+1];
        log += F(" BaseVar+2  : ");
        log += UserVar[event->BaseVarIndex+2];
        addLog(LOG_LEVEL_INFO,log);

                   
        success = true;
        break;
      }
  }
  return success;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////// Method to read peak to peak signal volts from CT sensor unit  //////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// This code runs at high frequency for the sampling period (currently 100 millisecs)...

        float Plugin_202_getVPP() {       
          float result;                   
          int readValue;             // instantaneous volt value read from the sensor
          int maxValue = 0;          // store max voltvalue here
          int minValue = 0;          // store min voltvalue here
          uint32_t start_time = millis();
          
          while((millis()-start_time) < 100) //sample for 100 milliSec, each cycle of mains is 1/50th sec = 20 millisec
            {
              readValue = analogRead(A0); // read digital signal representing instantaneous volt value from sensor (0-254)
              // see if we have a new maxValue
               if (readValue > maxValue) 
              {
                 // record the maximum sensor value
                maxValue = readValue;
              }
// Different versions of Henrys Bench code exist, some use max and minimum value, others use only maximum.
// It really depends how the circuitry in the sensor treats the AC waveform coming in via the C.T. which is unknown
// (unless someone can look with an oscilloscope at the signal output!)
// Try without minimum...
/*
              // see if we have a new minValue
               if (readValue < minValue) 
              {
                 // record the minimum sensor value
                minValue = readValue;
              }

*/              
            }
/*
            // subtract min from max to get peak to peak voltage
            result = maxValue - minValue;
*/
            result = maxValue;
         return result;
        }

