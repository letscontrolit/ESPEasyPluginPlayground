#ifndef PLUGINSTRUCTS_P200_DATA_STRUCT_H
#define PLUGINSTRUCTS_P200_DATA_STRUCT_H

#include "../../_Plugin_Helper.h"
#include "src/Helpers/StringConverter.h"

#ifdef USES_P200

#include <ESPeasySerial.h>
#include <base64.h>

# define P200_SERIAL_TIMEOUT     2

#define P200_OUTPUT_JSON        0
#define P200_OUTPUT_CSV         1
#define P200_OUTPUT_RAW         2

struct P200_data_struct : public PluginTaskData_base {
public:

  P200_data_struct();

  ~P200_data_struct();

  void reset();

  bool init(ESPEasySerialPort port, 
            const int16_t serial_rx,
            const int16_t serial_tx,
            unsigned long baudrate);

  bool isInitialized() const;

  bool loop();

  bool getSentence(String& string);

  void getSentencesReceived(uint32_t& succes,
                            uint32_t& error,
                            uint32_t& length_last) const;

  void setMaxLength(uint16_t maxlenght);

  void resetSerialTimeout();

  uint8_t output_type = 0;

private:

  bool max_length_reached() const;

  bool serialTimeout() const;

  bool isNumber(const String& str);

  ESPeasySerial *easySerial = nullptr;
  String         sentence_part;
  String         last_sentence;
  uint16_t       max_length               = 550;
  uint32_t       sentences_received       = 0;
  uint32_t       sentences_received_error = 0;
  uint32_t       length_last_received     = 0;
  unsigned long  serial_timeout           = 0;
  int8_t         sentence_checksum        = 0;

};

#endif // USES_P200

#endif // PLUGINSTRUCTS_P200_DATA_STRUCT_H
