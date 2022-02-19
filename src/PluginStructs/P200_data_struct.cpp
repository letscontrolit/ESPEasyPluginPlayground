#include "../PluginStructs/P200_data_struct.h"


// Needed also here for PlatformIO's library finder as the .h file 
// is in a directory which is excluded in the src_filter
#include <ESPeasySerial.h>
#include <base64.h>

#ifdef USES_P200


P200_data_struct::P200_data_struct() :  easySerial(nullptr) {}

P200_data_struct::~P200_data_struct() {
  reset();
}

void P200_data_struct::reset() {
  if (easySerial != nullptr) {
    delete easySerial;
    easySerial = nullptr;
  }
}

bool P200_data_struct::init(ESPEasySerialPort port, const int16_t serial_rx, const int16_t serial_tx, unsigned long baudrate) {
  if ((serial_rx < 0) && (serial_tx < 0)) {
    return false;
  }
  reset();
  easySerial = new (std::nothrow) ESPeasySerial(port, serial_rx, serial_tx);

  if (isInitialized()) {
    easySerial->begin(baudrate);
    return true;
  }
  return false;
}

bool P200_data_struct::isInitialized() const {
  return easySerial != nullptr;
}

bool P200_data_struct::loop() {
  if (!isInitialized()) {
    return false;
  }
  bool fullSentenceReceived = false;

  if (easySerial != nullptr) {
    int available = easySerial->available();

    while (available > 0 && !fullSentenceReceived) {
      char c = easySerial->read();
      --available;

      if (available == 0) {
        available = easySerial->available();
        if (available == 0) { resetSerialTimeout(); }
        delay(0);
      }

      switch (sentence_part.length()) {
        case 0:
          if (c != '\r') break; // wait for '\r'
        default:
          sentence_part += c;
          sentence_checksum -= c;
          break;
      }

      if (max_length_reached()) { fullSentenceReceived = true; }
    }
  }

  if (serialTimeout() || fullSentenceReceived) {
    const size_t length = sentence_part.length();
    bool valid          = length > 0;

    fullSentenceReceived = false;

    if (valid) {
    // message or full buffer
      
      
      if (output_type == P200_OUTPUT_RAW) {
        length_last_received = sentence_part.length();
        last_sentence = base64::encode(sentence_part);
        fullSentenceReceived = true;
        sentence_part = "";
        ++sentences_received;
      } else {
        // Cheksum
        sentence_checksum = sentence_checksum & 0xff;

        if (sentence_checksum != 0) {
          sentence_checksum = 0;
          sentence_part = "";
          ++sentences_received_error;
        } else {
          // JSON & CSV
          String field;
          uint16_t field_start = 2;
          uint16_t field_end = sentence_part.indexOf('\r', field_start);
          uint16_t field_separator = 0;
          if (output_type == P200_OUTPUT_JSON) { last_sentence = F("{"); }

          while (field_end <= length) {
            // this will drop last field (checksum) 
            if (last_sentence.length() >= 2) { last_sentence += F(","); }
            field = sentence_part.substring(field_start, field_end); // <Field-Labe><Tab><Field-Value>
            
            field_separator = field.indexOf('\t');

            switch (output_type) {
              case P200_OUTPUT_JSON:
                 last_sentence += to_json_object_value(field.substring(0, field_separator), field.substring(field_separator + 1), false);
                break;
              case P200_OUTPUT_CSV:
                 last_sentence += field.substring(0, field_separator) + F(":") + field.substring(field_separator + 1);
                break;
              default:
                break;
            }
            // if (output_type == P200_OUTPUT_JSON) { last_sentence += '\"'; }
            // last_sentence += field.substring(0, field_separator); // <Field-Label>
            // if (output_type == P200_OUTPUT_JSON) { last_sentence += '\"'; }
            // last_sentence += ':';

            // field = field.substring(field_separator + 1); // <Field-Value>
        
            // // Do we have <Field-Value>s incorrecly detected as numbers???
            // if (isNumber(field) || (output_type == P200_OUTPUT_CSV)) {
            //   last_sentence += field; 
            // } else {
            //   last_sentence += '\"' + field + '\"';
            // }
        
            field_start = field_end + 2;
            field_end = sentence_part.indexOf('\r', field_start);
            delay(0);
          }
          
          if (output_type == P200_OUTPUT_JSON) { last_sentence += F("}"); }
          length_last_received = sentence_part.length();
          fullSentenceReceived = true;
          sentence_part = "";
          ++sentences_received;
        }
      }
    }
  }

  return fullSentenceReceived;
}

bool P200_data_struct::getSentence(String& string) {
  string        = last_sentence;
  if (string.isEmpty()) {
    return false;
  }
  last_sentence = "";
  return true;
}

void P200_data_struct::getSentencesReceived(uint32_t& succes, uint32_t& error, uint32_t& length_last) const {
  succes      = sentences_received;
  error       = sentences_received_error;
  length_last = length_last_received;
}

void P200_data_struct::setMaxLength(uint16_t maxlenght) {
  max_length = maxlenght;
}

void P200_data_struct::resetSerialTimeout() {
    serial_timeout = millis() + P200_SERIAL_TIMEOUT;
}

bool P200_data_struct::serialTimeout() const {
  if (timeOutReached(serial_timeout)) {
    return true;
  }
  return false;
}

bool P200_data_struct::max_length_reached() const {
  if (max_length == 0) { return false; }
  return sentence_part.length() >= max_length;
}

bool P200_data_struct::isNumber(const String& str) {
  for (char const &c : str) {
    if (std::isdigit(c) == 0) return false;
  }
  return true;
}

#endif // USES_P200
