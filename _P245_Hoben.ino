#ifdef USES_P245
//#######################################################################################################
//#################################### Plugin 245: Hoben  ###############################################
//#######################################################################################################
//
// Usage: Connect an IR led to ESP8266 GPIO14 (D5) preferably. (various schematics can be found online)
// On the device tab add a new device and select "Regulator - Hoben"
// Enable the device and select the GPIO led pin
// Power on the ESP and connect to it
//
// Control of Hoben is done by sending a fake temperature :
// - To start heating, use HOBENSTART cmd
//    * timer is set to start duration
//    * a dummy temperature equal to Reference Temperature - Control offset is sent
//    * timer is decremented every minute until 1
//    * when timer is 1, min (real temperature,reference temperature) is sent 
// - To stop heating, use HOBENSTOP cmd
//    * timer is set to stop duration
//    * a dummy temperature equal to Reference Temperature + Control offset is sent
//    * timer is decremented every minute until -1
//    * when timer is -1, max (real temperature,reference temperature) is sent 
// - To enable auto mode (Hoben self control), use HOBENAUTO cmd
//    * real Temperature is sent

#include <IRsend.h>

IRsend *Plugin_245_irSender = nullptr;

#define PLUGIN_245
#define PLUGIN_ID_245 245
#define PLUGIN_NAME_245 "Regulator - Hoben [DEVELOPMENT]"
#define PLUGIN_VALUENAME1_245 "Mode"
#define PLUGIN_VALUENAME2_245 "Consigne"

#define PLUGIN_245_PULSE_LENGTH 480
#define PLUGIN_245_BLANK_LENGTH 510
#define PLUGIN_245_TEMP_MAX 35.0 
#define PLUGIN_245_TEMP_MIN 2.0

#define P245_Ntimings 32 //Defines the ammount of timings that can be stored. 

boolean Plugin_245(byte function, struct EventStruct *event, String &string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_245;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].DecimalsOnly = true;
        //Device[deviceCount].Custom = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_245);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_245));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_245));
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        event->String1 = formatGpioName_output("LED");
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormTextBox(F("Reference Temperature"), F("p245_reference"), String(PCONFIG_FLOAT(0), 1), 4);
        addUnit(F("°"));
        addFormTextBox(F("Temperature Offset"), F("p245_offset"), String(PCONFIG_FLOAT(2), 1), 4);
        addUnit(F("°"));
        addFormTextBox(F("Control Offset "), F("p245_controloffset"), String(PCONFIG_FLOAT(1), 1), 3);
        addUnit(F("°"));
        addFormNumericBox(F("Control Duration"), F("p245_duration"), PCONFIG(2), 1, 60);
        addUnit(F("min"));
        addHtml(F("<TR><TD>Check Task:<TD>"));
        addTaskSelect(F("p245_task"), PCONFIG(0));
        LoadTaskSettings(PCONFIG(0)); // we need to load the values from another task for selection!
        addHtml(F("<TR><TD>Check Value:<TD>"));
        addTaskValueSelect(F("p245_value"), PCONFIG(1), PCONFIG(0));
        LoadTaskSettings(event->TaskIndex); // we need to restore our original taskvalues!

        String cmd;
        addRowLabel(F("Command Start heating"));
        cmd = F("HOBENSTART,");
        cmd += event->TaskIndex + 1;
        addHtml(cmd);
        addRowLabel(F("Command Stop heating"));
        cmd = F("HOBENSTOP,");
        cmd += event->TaskIndex + 1;
        addHtml(cmd);
        addRowLabel(F("Command Auto Mode"));
        cmd = F("HOBENAUTO,");
        cmd += event->TaskIndex + 1;
        addHtml(cmd);
        addRowLabel(F("Set Regulation Temperature"));
        cmd = F("HOBENSETPOINT,");
        cmd += event->TaskIndex + 1;
        cmd += F(",value");
        addHtml(cmd);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        PCONFIG_FLOAT(0) = getFormItemFloat(F("p245_reference"));
        PCONFIG_FLOAT(1) = getFormItemFloat(F("p245_controloffset"));
        PCONFIG_FLOAT(2) = getFormItemFloat(F("p245_offset"));
        PCONFIG(0) = getFormItemInt(F("p245_task"));
        PCONFIG(1) = getFormItemInt(F("p245_value"));
        PCONFIG(2) = getFormItemInt(F("p245_duration"));

        UserVar[event->BaseVarIndex+1] = PCONFIG_FLOAT(0);
        success = true;
        break;
      }
      
    case PLUGIN_INIT:
      {
        int irPin = CONFIG_PIN1;
        if (Plugin_245_irSender == 0 && irPin != -1)
        {
          addLog(LOG_LEVEL_INFO, F("INIT: IR TX"));
          addLog(LOG_LEVEL_INFO, String(F("IR lib Version: ")) + _IRREMOTEESP8266_VERSION_);
          Plugin_245_irSender = new IRsend(irPin);
          Plugin_245_irSender->begin(); // Start the sender
        }
        if (Plugin_245_irSender != 0 && irPin == -1)
        {
          addLog(LOG_LEVEL_INFO, F("INIT: IR TX Removed"));
          delete Plugin_245_irSender;
          Plugin_245_irSender = 0;
        }

        // Init Vars
        //UserVar[event->BaseVarIndex] = 0;
        //UserVar[event->BaseVarIndex+1] = PCONFIG_FLOAT(0);
        
        success = true;
        break;
      }
      
    case PLUGIN_READ:
      {
        String log;
        if (Plugin_245_irSender == 0) {
          log = F("HOBEN : No IR LED, skipping: ");
          addLog(LOG_LEVEL_INFO, log);
          success = true;
          return success;
        }
        
        // we're checking a var from another task, so calculate that basevar
        byte TaskIndex = PCONFIG(0);
        byte BaseVarIndex = TaskIndex * VARS_PER_TASK + PCONFIG(1);
        
        log = F("HOBEN : Temperature of sensor: ");
        log += UserVar[BaseVarIndex];
        addLog(LOG_LEVEL_DEBUG, log);

        log = F("HOBEN : Offset: ");
        log += PCONFIG_FLOAT(2);
        addLog(LOG_LEVEL_DEBUG, log);
        
        // To avoid using float, use 10 multiplier in all the compute
        unsigned int value = 10 * ( UserVar[BaseVarIndex] + PCONFIG_FLOAT(2) ) ;

        if ( UserVar[event->BaseVarIndex+1] >0 ) {
          float setPointCorr =  PCONFIG_FLOAT(0) - UserVar[event->BaseVarIndex+1];
          log = F("HOBEN : SetPoint correction: ");
          log += setPointCorr;
          addLog(LOG_LEVEL_DEBUG, log);
          value += (unsigned int) 10 * setPointCorr;
        }

        log = F("HOBEN : Control offset: ");
        log += PCONFIG_FLOAT(1);
        addLog(LOG_LEVEL_DEBUG, log);
        
        //Mode control offset
        if ( UserVar[event->BaseVarIndex] > 1 ) {
          value = min(value, (unsigned int) (10 * ( PCONFIG_FLOAT(0) - PCONFIG_FLOAT(1) + PCONFIG_FLOAT(2))) );
        }
        else if ( UserVar[event->BaseVarIndex] == 1 ) {
          value = min(value, (unsigned int) (10 * ( PCONFIG_FLOAT(0) + PCONFIG_FLOAT(2))) );
        }
        else if ( UserVar[event->BaseVarIndex] < -1 ) {
          value = max(value, (unsigned int) (10 * ( PCONFIG_FLOAT(0) + PCONFIG_FLOAT(1) + PCONFIG_FLOAT(2))) );         
        }
        else if ( UserVar[event->BaseVarIndex] == -1 ) {
          value = max(value, (unsigned int) (10 * ( PCONFIG_FLOAT(0) + PCONFIG_FLOAT(2))) );
        }
               
        //Cleanup value
        value = min(value, (unsigned int) (10 * PLUGIN_245_TEMP_MAX) );
        value = max(value, (unsigned int) (10 * PLUGIN_245_TEMP_MIN) );

        log = F("HOBEN : Temperature to encode: ");
        log += value;
        addLog(LOG_LEVEL_DEBUG, log);
        
        uint16_t idx = 0; //If this goes above the buf.size then the esp will throw a 28 EXCCAUSE
        uint16_t *buf;
        buf = new uint16_t[P245_Ntimings]; //The Raw Timings that we can buffer.
        if (buf == nullptr)
        { // error assigning memory.
          success = false;
          return success;
        }

        // Build HOBEN Pattern
        // First Block will not be repeated
        unsigned int remainder;
        buf[idx++] = 3 * PLUGIN_245_PULSE_LENGTH; 
        if (value > 259) {
          log = F("HOBEN : High Temp, Compute (Temp - 259)");
          addLog(LOG_LEVEL_DEBUG, log);
          buf[idx++] = 5 * PLUGIN_245_BLANK_LENGTH; 
          buf[idx++] = 1 * PLUGIN_245_PULSE_LENGTH;
          buf[idx++] = 3 * PLUGIN_245_BLANK_LENGTH;
          remainder = value - 259;   
        }
        else {
          log = F("HOBEN : Low Temp, Compute (Temp + 238)");
          addLog(LOG_LEVEL_DEBUG, log);
          buf[idx++] = 9 * PLUGIN_245_BLANK_LENGTH; 
          remainder = value + 238;
        }
        // Remainder value will be send 2 times in 3 blocks
        log = F("HOBEN : remainder: ");
        log += remainder;
        addLog(LOG_LEVEL_DEBUG, log);
        
        unsigned int data3 = remainder / 62;
        if (data3 > 7)
          data3=7; // patch for round of computing (259+238)/62=8
        remainder = remainder - data3 * 62;
        unsigned int data2 = remainder / 8;
        unsigned int data1 = remainder - data2 * 8; 
        
        log = F("HOBEN : computed: ");
        log += data3;
        log += F(" * 62 + " );
        log += data2;
        log += F(" * 8 + " );
        log += data1;
        addLog(LOG_LEVEL_DEBUG, log);
        
        addValueToBuff(data1,buf,&idx);
        addValueToBuff(data2,buf,&idx); 
        addValueToBuff(data3,buf,&idx);
        addValueToBuff(data1,buf,&idx);
        addValueToBuff(data2,buf,&idx); 
        addValueToBuff(data3,buf,&idx);
        
        log = F("HOBEN : IR Timing: ");
        for (uint16_t i = 0; i < idx; i++) {
            log += buf[i];
            log += F(" ");     
        }
        addLog(LOG_LEVEL_DEBUG, log);
        
        #ifdef PLUGIN_016
            if (irReceiver != 0)
              irReceiver->disableIRIn(); // Stop the receiver
        #endif      
        Plugin_245_irSender->sendRaw(buf, idx, 38);
        delete[] buf;
        buf = nullptr;

        log = F("HOBEN : IR Code sent ");
        addLog(LOG_LEVEL_INFO, log);
        
        #ifdef PLUGIN_016
          if (irReceiver != 0)
            irReceiver->enableIRIn(); // Start the receiver
        #endif
        
        success = true;
        break;
      }
      
    case PLUGIN_WRITE:
      {
        String log = F("HOBEN PLUGIN WRITE : ");
        addLog(LOG_LEVEL_DEBUG_MORE, log);

        if (event->Par1 == event->TaskIndex+1) { // make sure that this instance is the target
          String command = parseString(string, 1);
          log = F("HOBEN CMD : ");
          log += command;
          addLog(LOG_LEVEL_DEBUG, log);
          
          if (command.equalsIgnoreCase(F("HOBENSTART"))) {
                UserVar[event->BaseVarIndex]=PCONFIG(2);
                success = true;
          }
          else if (command.equalsIgnoreCase(F("HOBENSTOP"))) {
                UserVar[event->BaseVarIndex]=-PCONFIG(2);
                success = true;
          }
         else if (command.equalsIgnoreCase(F("HOBENAUTO"))) {
                UserVar[event->BaseVarIndex]=0;
                success = true;
          }
         else if (command.equalsIgnoreCase(F("HOBENSETPOINT"))) {
                String param = parseString(string, 3);
                log = F("param : ");
                log += param;
                addLog(LOG_LEVEL_DEBUG, log);
                UserVar[event->BaseVarIndex+1]= param.toFloat();
                success = true;
          }
        }

        break;
      }

    case PLUGIN_CLOCK_IN:
    {
      String log = F("HOBEN : CLOCK: ");
      addLog(LOG_LEVEL_DEBUG_MORE, log);
      
      if ( UserVar[event->BaseVarIndex]>1) {
        UserVar[event->BaseVarIndex]--;
      }
      else if ( UserVar[event->BaseVarIndex]<-1) {
        UserVar[event->BaseVarIndex]++;       
      }
      success = true;
      break;
    }
      
  } // SWITCH END
  return success;
} // Plugin_245 END

void addValueToBuff(unsigned int value, uint16_t *lbuf, uint16_t *pos) {
        String log = F("HOBEN : Encoding Data: ");
        log += value; 
        addLog(LOG_LEVEL_DEBUG_MORE, log);

        if ( ( value == 0 ) || (value > 7 ) ) {
          lbuf[(*pos)++] = 2 * PLUGIN_245_PULSE_LENGTH;
          lbuf[(*pos)++] = 10 * PLUGIN_245_BLANK_LENGTH; 
        }
        else {
          lbuf[(*pos)++] = 1 * PLUGIN_245_PULSE_LENGTH;
          lbuf[(*pos)++] = value * PLUGIN_245_BLANK_LENGTH; 
          lbuf[(*pos)++] = 1 * PLUGIN_245_PULSE_LENGTH;
          lbuf[(*pos)++] = (10 - value)  * PLUGIN_245_BLANK_LENGTH; 
        }
}

#endif // USES_P245
