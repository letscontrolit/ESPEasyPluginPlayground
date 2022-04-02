#include "_Plugin_Helper.h"
#ifdef USES_P249

// ####################################################################################################
// ############################# Plugin 249: GoodWe PV inverter XS-series #############################
// ####################################################################################################
//
//  Written by Henri de Jong (espeasy@enri.nl),
//      with most code inspired by plugin 224: _P224_DDS238.ino

#ifndef USES_MODBUS
#error This code needs MODBUS library, it should be enabled in 'define_plugin_sets.h', or your 'custom.h'
#endif

/*
   Circuit wiring
    GPIO Setting 1 -> RX (RO) (D6)
    GPIO Setting 2 -> TX (DI) (D7)
    GPIO Setting 3 -> DE/RE pin for MAX485 (D5)
    Use 1kOhm in serie on datapins!
 */

#define PLUGIN_249
#define PLUGIN_ID_249 249
#define PLUGIN_NAME_249 "GoodWe PV Inverter XS-series [DEVELOPMENT]"
#define PLUGIN_VALUENAME1_249 "TotalEnergy"  // 0313
#define PLUGIN_VALUENAME2_249 "FeedingPower" // 0233
#define PLUGIN_VALUENAME3_249 "Temperature"  // 0235
#define PLUGIN_VALUENAME4_249 "DayEnergy"    // 0236
#define PLUGIN_VALUENAME5_249 "E-Day"        // 0320
#define PLUGIN_VALUENAME6_249 "Pac"          // 0353
#define PLUGIN_VALUENAME7_249 "TempInternal" // 030F

#define P249_DEV_ID         PCONFIG(0)
#define P249_DEV_ID_LABEL   PCONFIG_LABEL(0)
#define P249_BAUDRATE       PCONFIG(2)
#define P249_BAUDRATE_LABEL PCONFIG_LABEL(2)
#define P249_QUERY1         PCONFIG(3)
#define P249_QUERY2         PCONFIG(4)
#define P249_QUERY3         PCONFIG(5)
#define P249_QUERY4         PCONFIG(6)
#define P249_DEPIN          CONFIG_PIN3

#define P249_NR_OUTPUT_VALUES   VARS_PER_TASK
#define P249_QUERY1_CONFIG_POS  3

#define P249_QUERY_TOTAL        0 // Total Energy
#define P249_QUERY_FEEDING      1 // Feeding Power
#define P249_QUERY_TEMP         2 // Temperature
#define P249_QUERY_DAY          3 // Day Energy
#define P249_QUERY_EDAY         4 // Day Energy
#define P249_QUERY_PAC          5 // P ac
#define P249_QUERY_TEMP_INT     6 // Internal temperature
#define P249_NR_OUTPUT_OPTIONS  7 // Must be the last one

#define P249_DEV_ID_DFLT   247     // Modbus communication address
#define P249_BAUDRATE_DFLT 3       // 9600 baud
#define P249_QUERY1_DFLT   P249_QUERY_TOTAL
#define P249_QUERY2_DFLT   P249_QUERY_FEEDING
#define P249_QUERY3_DFLT   P249_QUERY_TEMP
#define P249_QUERY4_DFLT   P249_QUERY_DAY

#define P249_MEASUREMENT_INTERVAL 60000L

#include <ESPeasySerial.h>
#include "src/Helpers/Modbus_RTU.h"

struct P249_data_struct : public PluginTaskData_base {
  P249_data_struct() {}

  ~P249_data_struct() {
    reset();
  }

  void reset() {
    modbus.reset();
  }

  bool init(ESPEasySerialPort port, const int16_t serial_rx, const int16_t serial_tx, int8_t dere_pin,
            unsigned int baudrate, uint8_t modbusAddress) {
    return modbus.init(port, serial_rx, serial_tx, baudrate, modbusAddress, dere_pin);
  }

  bool isInitialized() const {
    return modbus.isInitialized();
  }

  ModbusRTU_struct modbus;
};

unsigned int _plugin_249_last_measurement = 0;

boolean Plugin_249(byte function, struct EventStruct *event, String& string) {
  boolean success = false;

  switch (function) {
    case PLUGIN_DEVICE_ADD: {
      Device[++deviceCount].Number           = PLUGIN_ID_249;
      Device[deviceCount].Type               = DEVICE_TYPE_SERIAL_PLUS1; // connected through 3 datapins
      Device[deviceCount].VType              = Sensor_VType::SENSOR_TYPE_QUAD;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = true;
      Device[deviceCount].ValueCount         = P249_NR_OUTPUT_VALUES;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].GlobalSyncOption   = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME: {
      string = F(PLUGIN_NAME_249);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES: {
      for (byte i = 0; i < P249_NR_OUTPUT_VALUES; ++i) {
        if (i < P249_NR_OUTPUT_VALUES) {
          const byte pconfigIndex = i + P249_QUERY1_CONFIG_POS;
          byte choice             = PCONFIG(pconfigIndex);
          safe_strncpy(
            ExtraTaskSettings.TaskDeviceValueNames[i],
            Plugin_249_valuename(choice, false),
            sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
        } else {
          ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
        }
      }
      break;
    }

    case PLUGIN_GET_DEVICEGPIONAMES: {
      serialHelper_getGpioNames(event);
      event->String3 = formatGpioName_output_optional("DE");
      break;
    }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
    {
      string += serialHelper_getSerialTypeLabel(event);
      success = true;
      break;
    }

    case PLUGIN_SET_DEFAULTS: {
      P249_DEV_ID   = P249_DEV_ID_DFLT;
      P249_BAUDRATE = P249_BAUDRATE_DFLT;
      P249_QUERY1   = P249_QUERY1_DFLT;
      P249_QUERY2   = P249_QUERY2_DFLT;
      P249_QUERY3   = P249_QUERY3_DFLT;
      P249_QUERY4   = P249_QUERY4_DFLT;

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SHOW_SERIAL_PARAMS:
    {
        String options_baudrate[4];

        for (int i = 0; i < 4; ++i) {
          options_baudrate[i] = String(P249_storageValueToBaudrate(i));
        }
        addFormSelector(F("Baud Rate"), P249_BAUDRATE_LABEL, 4, options_baudrate, NULL, P249_BAUDRATE);
        addUnit(F("baud"));
        addFormNumericBox(F("Modbus Address"), P249_DEV_ID_LABEL, P249_DEV_ID, 1, 247);
      break;
    }

    case PLUGIN_WEBFORM_LOAD: {
      P249_data_struct *P249_data =
        static_cast<P249_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr != P249_data) && P249_data->isInitialized()) {
        String detectedString = P249_data->modbus.detected_device_description;

        if (detectedString.length() > 0) {
          addFormNote(detectedString);
        }
        addRowLabel(F("Checksum (pass/fail/nodata)"));
        uint32_t reads_pass, reads_crc_failed, reads_nodata;
        P249_data->modbus.getStatistics(reads_pass, reads_crc_failed, reads_nodata);
        String chksumStats;
        chksumStats  = reads_pass;
        chksumStats += '/';
        chksumStats += reads_crc_failed;
        chksumStats += '/';
        chksumStats += reads_nodata;
        addHtml(chksumStats);

        addFormSubHeader(F("Logged Values"));
        P249_showValueLoadPage(P249_QUERY_TOTAL, event);
        P249_showValueLoadPage(P249_QUERY_FEEDING, event);
        P249_showValueLoadPage(P249_QUERY_TEMP, event);
        P249_showValueLoadPage(P249_QUERY_DAY, event);
      }

      {
        // In a separate scope to free memory of String array as soon as possible
        sensorTypeHelper_webformLoad_header();
        String options[P249_NR_OUTPUT_OPTIONS];

        for (int i = 0; i < P249_NR_OUTPUT_OPTIONS; ++i) {
          options[i] = Plugin_249_valuename(i, true);
        }

        for (byte i = 0; i < P249_NR_OUTPUT_VALUES; ++i) {
          const byte pconfigIndex = i + P249_QUERY1_CONFIG_POS;
          sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P249_NR_OUTPUT_OPTIONS, options);
        }
      }
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      // Save normal parameters
      for (int i = 0; i < P249_QUERY1_CONFIG_POS; ++i) {
        pconfig_webformSave(event, i);
      }

      // Save output selector parameters.
      for (byte i = 0; i < P249_NR_OUTPUT_VALUES; ++i) {
        const byte pconfigIndex = i + P249_QUERY1_CONFIG_POS;
        const byte choice       = PCONFIG(pconfigIndex);
        sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, Plugin_249_valuename(choice, false));
      }

      success = true;
      break;
    }

    case PLUGIN_INIT: {
      const int16_t serial_rx = CONFIG_PIN1;
      const int16_t serial_tx = CONFIG_PIN2;
      const ESPEasySerialPort port = static_cast<ESPEasySerialPort>(CONFIG_PORT);
      initPluginTaskData(event->TaskIndex, new (std::nothrow) P249_data_struct());
      P249_data_struct *P249_data =
        static_cast<P249_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == P249_data) {
        return success;
      }

      if (P249_data->init(port, serial_rx, serial_tx, P249_DEPIN,
                          P249_storageValueToBaudrate(P249_BAUDRATE),
                          P249_DEV_ID)) {
        serialHelper_log_GpioDescription(port, serial_rx, serial_tx);
        success = true;
      } else {
        clearPluginTaskData(event->TaskIndex);
      }
      break;
    }

    case PLUGIN_EXIT: {
      success = true;
      break;
    }

    case PLUGIN_READ: {
      P249_data_struct *P249_data =
        static_cast<P249_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((nullptr != P249_data) && P249_data->isInitialized()) {
        for (int i = 0; i < P249_NR_OUTPUT_VALUES; ++i) {
          UserVar[event->BaseVarIndex + i] = P249_readValue(PCONFIG(i + P249_QUERY1_CONFIG_POS), event);
          delay(1);
        }

        success = true;
      }
      break;
    }
  }
  return success;
}

String Plugin_249_valuename(byte value_nr, bool displayString) {
  switch (value_nr) {
    case P249_QUERY_TOTAL: return displayString ? F("Total Energy (Wh)") : F("TotalEnergy");
    case P249_QUERY_FEEDING: return displayString ? F("Feeding Power (W)") : F("FeedingW");
    case P249_QUERY_TEMP: return displayString ? F("Temperature (C)") : F("Temp");
    case P249_QUERY_DAY: return displayString ? F("Day Energy (Wh)") : F("DayEnergy");
    case P249_QUERY_EDAY: return displayString ? F("E-Day (Wh)") : F("E-Day");
    case P249_QUERY_PAC: return displayString ? F("P ac (W)") : F("Pac");
    case P249_QUERY_TEMP_INT: return displayString ? F("Internal temp (C)") : F("InternalTemp");
  }
  return "";
}

int P249_storageValueToBaudrate(byte baudrate_setting) {
  switch (baudrate_setting) {
    case 0:
      return 1200;
    case 1:
      return 2400;
    case 2:
      return 4800;
    case 3:
      return 9600;
  }
  return 9600;
}

float P249_readValue(byte query, struct EventStruct *event) {
  byte errorcode = -1;
  float value = 0;
  P249_data_struct *P249_data =
    static_cast<P249_data_struct *>(getPluginTaskData(event->TaskIndex));

  if ((nullptr != P249_data) && P249_data->isInitialized()) {
    switch (query) {
      case P249_QUERY_TOTAL:
        value = P249_data->modbus.readHoldingRegister(0x0313, errorcode) * 100.0f; // 0.1 kWh => Wh
        break;
      case P249_QUERY_FEEDING:
        value = P249_data->modbus.readHoldingRegister(0x0233, errorcode) * 1.0f; // 1 W => 1 W
        break;
      case P249_QUERY_TEMP:
        value =  P249_data->modbus.readHoldingRegister(0x0235, errorcode) / 10.0f; // 0.1 C => C
        break;
      case P249_QUERY_DAY:
        value = P249_data->modbus.readHoldingRegister(0x0236, errorcode) * 100.0f; // 0.1 kW => W
        break;
      case P249_QUERY_EDAY:
        value = P249_data->modbus.readHoldingRegister(0x0320, errorcode) * 100.0f; // 0.1 kWh => Wh
        break;
      case P249_QUERY_PAC:
        value = P249_data->modbus.readHoldingRegister(0x0353, errorcode) * 1.0f; // 1 W => W
        break;
      case P249_QUERY_TEMP_INT:
        value = P249_data->modbus.readHoldingRegister(0x030F, errorcode) / 10.0f; // 0.1 C => C
        break;
    }
  }
  if (errorcode == 0) return value;
  return 0.0f;
}

void P249_showValueLoadPage(byte query, struct EventStruct *event) {
  addRowLabel(Plugin_249_valuename(query, true));
  addHtml(String(P249_readValue(query, event)));
}

#endif // USES_P249
