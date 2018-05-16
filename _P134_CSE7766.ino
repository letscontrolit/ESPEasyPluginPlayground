#ifdef USES_P134
//#######################################################################################################
//###################### Plugin 134: CSE7766 - Energy (Sonoff S31 and Sonoff Pow R2) ####################
//#######################################################################################################
//###################################### stefan@clumsy.ch      ##########################################
//#######################################################################################################

#define PLUGIN_134
#define PLUGIN_ID_134         134
#define PLUGIN_NAME_134       "Voltage & Current (AC) - CSE7766 [TESTING]"
#define PLUGIN_VALUENAME1_134 "Voltage"
#define PLUGIN_VALUENAME2_134 "Power"
#define PLUGIN_VALUENAME3_134 "Current"
#define PLUGIN_VALUENAME4_134 "Pulses"

boolean Plugin_134_init = false;

#define CSE_NOT_CALIBRATED          0xAA
#define CSE_PULSES_NOT_INITIALIZED  -1
#define CSE_PREF                    1000
#define CSE_UREF                    100
#define HLW_PREF_PULSE         12530        // was 4975us = 201Hz = 1000W
#define HLW_UREF_PULSE         1950         // was 1666us = 600Hz = 220V
#define HLW_IREF_PULSE         3500         // was 1666us = 600Hz = 4.545A

unsigned long energy_power_calibration = HLW_PREF_PULSE;
unsigned long energy_voltage_calibration = HLW_UREF_PULSE;
unsigned long energy_current_calibration = HLW_IREF_PULSE;

uint8_t cse_receive_flag = 0;

long voltage_cycle = 0;
long current_cycle = 0;
long power_cycle = 0;
long power_cycle_first = 0;
long cf_pulses = 0;
long cf_pulses_last_time = CSE_PULSES_NOT_INITIALIZED;
long cf_frequency = 0;
uint8_t serial_in_buffer[32];
uint8_t serial_in_byte_counter = 0;
uint8_t serial_in_byte = 0;
float energy_voltage = 0;         // 123.1 V
float energy_current = 0;         // 123.123 A
float energy_power = 0;           // 123.1 W


boolean Plugin_134(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_134;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_134);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_134));
        break;
      }


    case PLUGIN_INIT:
      {
        Plugin_134_init = true;
        Serial.begin(4800);
        success = true;
        break;
      }

/* currently not needed!              
   case PLUGIN_TEN_PER_SECOND:
      {

        long cf_frequency = 0;
         
        if (CSE_PULSES_NOT_INITIALIZED == cf_pulses_last_time) {
          cf_pulses_last_time = cf_pulses;  // Init after restart
        } else {
          if (cf_pulses < cf_pulses_last_time) {  // Rolled over after 65535 pulses
            cf_frequency = (65536 - cf_pulses_last_time) + cf_pulses;
          } else {
            cf_frequency = cf_pulses - cf_pulses_last_time;
          }
          if (cf_frequency)  {
            cf_pulses_last_time = cf_pulses;
 //           energy_kWhtoday_delta += (cf_frequency * energy_power_calibration) / 36;
 //           EnergyUpdateToday();
          }
        }
        success = true;
        break;
      }
*/

   case PLUGIN_READ:
      {
        // We do not actually read as this is already done by reading the serial output
        // Instead we just send the last known state stored in Uservar
        addLog(LOG_LEVEL_DEBUG_DEV, F("CSE: plugin read"));
//        sendData(event);
        success = true;
        break;
      }


    case PLUGIN_SERIAL_IN:
      {
        if (Plugin_134_init)
        {
          byte val = 0;
          byte code[6];
          byte checksum = 0;
          byte bytesread = 0;
          byte tempbyte = 0;
  

          if (Serial.available() > 0) {
            serial_in_byte=Serial.read();
            success = true;
          }
          
          if (cse_receive_flag) {
            serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
            if (24 == serial_in_byte_counter) {
              addLog(LOG_LEVEL_DEBUG_DEV, F("CSE: packet received"));
              CseReceived();
              cse_receive_flag = 0;
              break;
            }
          } else {
            if (0x5A == serial_in_byte) {             // 0x5A - Packet header 2
              cse_receive_flag = 1;
              addLog(LOG_LEVEL_DEBUG_DEV, F("CSE: Header received"));
            } else {
              serial_in_byte_counter = 0;
            }
            serial_in_buffer[serial_in_byte_counter++] = serial_in_byte;
          }
          serial_in_byte = 0;                         // Discard

          UserVar[event->BaseVarIndex] = energy_voltage;
          UserVar[event->BaseVarIndex+1] = energy_power;
          UserVar[event->BaseVarIndex+2] = energy_current;
          UserVar[event->BaseVarIndex+3] = cf_pulses;

//          String log = F("Variable: Tag: ");
//          log += key;
//          addLog(LOG_LEVEL_INFO, log);
//          success = true;
          }
        break;
      }
  }
  return success;
}

void CseReceived()
{
  String log;
  uint8_t header = serial_in_buffer[0];
  if ((header & 0xFC) == 0xFC) {
    addLog(LOG_LEVEL_DEBUG, F("CSE: Abnormal hardware"));
    return;
  }

  // Calculate checksum
  uint8_t checksum = 0;
  for (byte i = 2; i < 23; i++) checksum += serial_in_buffer[i];
  if (checksum != serial_in_buffer[23]) {
    addLog(LOG_LEVEL_DEBUG, F("CSE: Checksum Failure"));
    return;
  }


  // Get chip calibration data (coefficients) and use as initial defaults
  if (HLW_UREF_PULSE == energy_voltage_calibration) {
    long voltage_coefficient = 191200;  // uSec
    if (CSE_NOT_CALIBRATED != header) {
      voltage_coefficient = serial_in_buffer[2] << 16 | serial_in_buffer[3] << 8 | serial_in_buffer[4];
    }
    energy_voltage_calibration = voltage_coefficient / CSE_UREF;
  }
  if (HLW_IREF_PULSE == energy_current_calibration) {
    long current_coefficient = 16140;  // uSec
    if (CSE_NOT_CALIBRATED != header) {
      current_coefficient = serial_in_buffer[8] << 16 | serial_in_buffer[9] << 8 | serial_in_buffer[10];
    }
    energy_current_calibration = current_coefficient;
  }
  if (HLW_PREF_PULSE == energy_power_calibration) {
    long power_coefficient = 5364000;  // uSec
    if (CSE_NOT_CALIBRATED != header) {
      power_coefficient = serial_in_buffer[14] << 16 | serial_in_buffer[15] << 8 | serial_in_buffer[16];
    }
    energy_power_calibration = power_coefficient / CSE_PREF;
  }


  uint8_t adjustement = serial_in_buffer[20];
  voltage_cycle = serial_in_buffer[5] << 16 | serial_in_buffer[6] << 8 | serial_in_buffer[7];
  current_cycle = serial_in_buffer[11] << 16 | serial_in_buffer[12] << 8 | serial_in_buffer[13];
  power_cycle = serial_in_buffer[17] << 16 | serial_in_buffer[18] << 8 | serial_in_buffer[19];
  cf_pulses = serial_in_buffer[21] << 8 | serial_in_buffer[22];

  log = F("CSE: adjustement ");
  log += adjustement;
  addLog(LOG_LEVEL_DEBUG_DEV, log);
  log = F("CSE: voltage_cycle ");
  log += voltage_cycle;
  addLog(LOG_LEVEL_DEBUG_DEV, log);
  log = F("CSE: current_cycle ");
  log += current_cycle;
  addLog(LOG_LEVEL_DEBUG_DEV, log);
  log = F("CSE: power_cycle ");
  log += power_cycle;
  addLog(LOG_LEVEL_DEBUG_DEV, log);
  log = F("CSE: cf_pulses ");
  log += cf_pulses;
  addLog(LOG_LEVEL_DEBUG_DEV, log);

//  if (energy_power_on) {  // Powered on

    if (adjustement & 0x40) {  // Voltage valid
      energy_voltage = (float)(energy_voltage_calibration * CSE_UREF) / (float)voltage_cycle;
    }
    if (adjustement & 0x10) {  // Power valid
      if ((header & 0xF2) == 0xF2) {  // Power cycle exceeds range
        energy_power = 0;
      } else {
        if (0 == power_cycle_first) power_cycle_first = power_cycle;  // Skip first incomplete power_cycle
        if (power_cycle_first != power_cycle) {
          power_cycle_first = -1;
          energy_power = (float)(energy_power_calibration * CSE_PREF) / (float)power_cycle;
        } else {
          energy_power = 0;
        }
      }
    } else {
      power_cycle_first = 0;
      energy_power = 0;  // Powered on but no load
    }
    if (adjustement & 0x20) {  // Current valid
      if (0 == energy_power) {
        energy_current = 0;
      } else {
        energy_current = (float)energy_current_calibration / (float)current_cycle;
      }
    }

// } else {  // Powered off
//    power_cycle_first = 0;
//    energy_voltage = 0;
//    energy_power = 0;
//    energy_current = 0;
//  }

  log = F("CSE voltage: ");
  log += energy_voltage;
  addLog(LOG_LEVEL_DEBUG, log);
  log = F("CSE power: ");
  log += energy_power;
  addLog(LOG_LEVEL_DEBUG, log);
  log = F("CSE current: ");
  log += energy_current;
  addLog(LOG_LEVEL_DEBUG, log);
  log = F("CSE piulses: ");
  log += cf_pulses;
  addLog(LOG_LEVEL_DEBUG, log);

}

#endif // USES_P134
