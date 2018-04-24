//#######################################################################################################
//################ Plugin 183:SMA STP modbus  inverter current power                  ###################
//#######################################################################################################
/*
 * this plugin brings its own modbus class as there is none in ESPEasy at the time of implementation
 * This may change soon.
 */

# ifndef MODBUS_H
# define MODBUS_H

enum MODBUS_states_t {MODBUS_IDLE, MODBUS_RECEIVE, MODBUS_RECEIVE_PAYLOAD};
enum MODBUS_registerTypes_t {signed16, unsigned16, signed32, unsigned32, signed64, unsigned64};

#define MODBUS_FUNCTION_READ 4

class Modbus
{
  public:
    Modbus(void);
    bool handle();
    bool begin(uint8_t function, uint8_t ModbusID, uint16_t ModbusRegister, MODBUS_registerTypes_t type, char* IPaddress);
    double read() {
      if (resultReceived) {
        resultReceived = false;
        return result;
      }
      else
        return -1;
    };
    bool available() {
      return resultReceived;
    };
    unsigned int getReadErrors() {
      return errcnt;
    };
    void resetReadErrors() {
      errcnt = 0;
    };
    void stop() {
      TXRXstate = MODBUS_IDLE;
      handle();
    };
    bool tryRead (uint8_t ModbusID, uint16_t M_register,  MODBUS_registerTypes_t type, char* IPaddress, double &result); 
    
  private:
    WiFiClient *ModbusClient; // pointer to tcp client
    unsigned int errcnt;
    char sendBuffer[12] =  {0, 1, 0, 0, 0, 6, 0x7e, 4, 0x9d, 7, 0, 1};
    String LogString = "";    // for debug logging
    unsigned long timeout;    // send and read timeout
    MODBUS_states_t TXRXstate;// state for handle() state machine
    unsigned int payLoad;     // number of bytes to receive as payload. Payload may come as seperate frame.
    bool hasTimeout();
    MODBUS_registerTypes_t incomingValue; // how to interpret the incoming value
    double result;                        // incoming value, converted to double
    bool resultReceived;                  // incoming value is valid ?
    bool isBusy(void) {
      return !(TXRXstate == MODBUS_IDLE);
    };
    uint16_t currentRegister;
    uint8_t currentFunction;

};
#endif



Modbus::Modbus() {
  TXRXstate = MODBUS_IDLE;
  timeout = 0;
  errcnt = 0;
  payLoad = 0;
  ModbusClient = nullptr;
  resultReceived=false;
}


bool Modbus::begin(uint8_t function, uint8_t ModbusID, uint16_t ModbusRegister,  MODBUS_registerTypes_t type, char* IPaddress)
{
  currentRegister = ModbusRegister;
  currentFunction = function;
  incomingValue = type;
  resultReceived = false;
  ModbusClient = new WiFiClient();
  ModbusClient->setNoDelay(true);
  ModbusClient->setTimeout(200);
  timeout = millis();
  ModbusClient->flush();

  if (ModbusClient->connected()) {
    LogString += F(" already connected. ");
  } else {
    LogString += F("connect: ");      LogString += IPaddress;
    if ( !ModbusClient->connect(IPaddress, 502)) {
      LogString += F(" fail. ");
      TXRXstate = MODBUS_IDLE;
      errcnt++;
      if (LogString.length() > 1 ) addLog(LOG_LEVEL_DEBUG, LogString);
      return false;
    }
  }
  LogString += F(" OK, sending read request: ");

  sendBuffer[6] = ModbusID ;
  sendBuffer[7] = function;
  sendBuffer[8] = (ModbusRegister >> 8) ;
  sendBuffer[9] = (ModbusRegister & 0x00ff) ;
  if ((incomingValue == signed64) || (incomingValue == unsigned64))
    sendBuffer[11] = 4;
  if ((incomingValue == signed32) || (incomingValue == unsigned32))
    sendBuffer[11] = 2;
  if ((incomingValue == signed16) || (incomingValue == unsigned16))
    sendBuffer[11] = 1;
  ModbusClient->flush();
  ModbusClient->write(&sendBuffer[0], sizeof(sendBuffer));
  for (unsigned int i = 0; i < sizeof(sendBuffer); i++) {
    LogString += ((unsigned int)(sendBuffer[i]));
    LogString += (" ");
  }
  TXRXstate = MODBUS_RECEIVE;
  if (LogString.length() > 1 ) addLog(LOG_LEVEL_DEBUG, LogString);
  return true;
}

bool Modbus::handle() {
  unsigned int RXavailable = 0;
  LogString = "";
  int64_t rxValue = 0;
  switch ( TXRXstate ) {

    case MODBUS_IDLE:
      // clean up;
      if (ModbusClient) {
        ModbusClient->flush();
        ModbusClient->stop();
        delete (ModbusClient);
        delay(1);
        ModbusClient = nullptr;
      }
      break;

    case MODBUS_RECEIVE:
      if  (hasTimeout())  break;
      if  (ModbusClient->available() < 9)  break;

      LogString += F("reading bytes: ");
      for (int a = 0; a < 9; a++) {
        payLoad = ModbusClient->read();
        LogString += (payLoad);  LogString += F(" ");
      }
      LogString += F("> ");
      if (payLoad > 8) {
        LogString += "Payload too large !? ";
        errcnt++;
        TXRXstate = MODBUS_IDLE;
      }

    case MODBUS_RECEIVE_PAYLOAD:
      if  (hasTimeout())  break;
      RXavailable = ModbusClient->available();
      if (payLoad != RXavailable) {
        TXRXstate = MODBUS_RECEIVE_PAYLOAD;
        break;
      }
      for (unsigned int i = 0; i < RXavailable; i++) {
        rxValue = rxValue << 8;
        char a = ModbusClient->read();
        rxValue = rxValue | a;
        LogString += ((int)a);  LogString += (" ");
      }
      switch (incomingValue) {
        case signed16:
          result = (int16_t) rxValue;
          break;
        case unsigned16:
          result = (uint16_t) rxValue;
          break;
        case signed32:
          result = (int32_t) rxValue;
          break;
        case unsigned32:
          result = (uint32_t) rxValue;
          break;
        case signed64:
          result = (int64_t) rxValue;
          break;
        case unsigned64:
          result = (uint64_t) rxValue;
          break;
      }

      LogString += "value: "; LogString += result;
      //if ((Settings.UseNTP) && (hour() == 0)) errcnt = 0;

      TXRXstate = MODBUS_IDLE;

      resultReceived = true;
      break;

    default:
      LogString += F("default. ");
      TXRXstate = MODBUS_IDLE;
      break;

  }
  if (LogString.length() > 1 ) addLog(LOG_LEVEL_DEBUG, LogString);
  return true;
}

bool Modbus::hasTimeout()
{
  if   ( (millis() - timeout) > 10000) { // too many bytes or timeout
    LogString += F("Modbus RX timeout. "); LogString += String(ModbusClient->available());
    errcnt++;
    TXRXstate = MODBUS_IDLE;
    return true;
  }
  return false;
}



// tryread can be called in a round robin fashion. It will initiate a read if Modbus is idle and update the result once it is available.
// subsequent calls (if Modbus is busy etc. ) will return false and not update the result.
// Use to read multiple values non blocking in an re-entrant function. Not tested yet.
bool Modbus::tryRead (uint8_t ModbusID, uint16_t M_register,  MODBUS_registerTypes_t type, char* IPaddress, double &result) {
  handle();
   //    Serial.println(" busy ?");
  if (isBusy()) return false;                                 // not done yet
  //    Serial.println("not busy");
  if (available()) {
   //    Serial.println("available");
   if ((currentFunction == MODBUS_FUNCTION_READ ) && (currentRegister == M_register)) {
      result = read();                                  // result belongs to this request.
    //       Serial.println("read done");
      return true;
    }
  } else {
  //  Serial.println("begin");
    begin(MODBUS_FUNCTION_READ, ModbusID, M_register, type, IPaddress);             // idle and no result -> begin read request
  }
  return false;
}


//#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_183
#define PLUGIN_ID_183         97
#define PLUGIN_NAME_183       "SMA_STPx000TL"
#define PLUGIN_VALUENAME1_183 "Power"
#define PLUGIN_VALUENAME2_183 "Energy"
#define PLUGIN_VALUENAME3_183 "CommErr"
#include <ESP8266WiFi.h>
 
char P183_IPaddress[20];

//==============================================
// SMA LIBRARY - SMA.h
// =============================================
# ifndef SMA_H
# define SMA_H

enum SMA_states_t { SMA_RECEIVE_POWER, SMA_RECEIVE_POWER_PENDING, SMA_RECEIVEPOWER_DONE, SMA_RECEIVE_ENERGY,  SMA_RECEIVE_ENERGY_PENDING, SMA_RECEIVE_ENERGY_DONE};

class SMA {
  private:
    Modbus *SMAModbus;
    SMA_states_t state;
    const char* IP;
    uint8_t ID;
    uint32_t timeout;
  public:
    SMA();
    uint16_t getModbusErrors() {
      return SMAModbus->getReadErrors();
    };
    double SMA_Power;
    double SMA_Energy;
    void handle();

};

#endif

void SMA::handle() {
  // use this to read in a normal way with 2sec pause between each modbus command, non blocking.
  SMAModbus->handle();
  switch (state) {
    case SMA_RECEIVE_POWER:
      SMAModbus->begin(MODBUS_FUNCTION_READ, 3, 30775, signed32, P183_IPaddress); // Power
      timeout = millis();
      state = SMA_RECEIVE_POWER_PENDING;
      break;
    case SMA_RECEIVE_POWER_PENDING:
      if (SMAModbus->available()) {
        SMA_Power = SMAModbus->read();
        timeout = millis();
        state = SMA_RECEIVEPOWER_DONE;
      }
      if (timeOutReached(timeout + 10000)) {
        SMAModbus->stop();
        state = SMA_RECEIVE_ENERGY;
      }
      break;
    case SMA_RECEIVEPOWER_DONE:
      if (timeOutReached(timeout + 2000))
        state = SMA_RECEIVE_ENERGY;
      break;
    case SMA_RECEIVE_ENERGY:
      timeout = millis();
      SMAModbus->begin(MODBUS_FUNCTION_READ, 3, 30517, unsigned64, P183_IPaddress); // energy
      state = SMA_RECEIVE_ENERGY_PENDING;
      break;
    case SMA_RECEIVE_ENERGY_PENDING:
      if (SMAModbus->available()) {
        SMA_Energy = SMAModbus->read();
        timeout = millis();
        state = SMA_RECEIVE_ENERGY_DONE;
      }
      if (timeOutReached(timeout + 10000)) {
        SMAModbus->stop();
        state = SMA_RECEIVE_POWER;
      }
      break;
    case SMA_RECEIVE_ENERGY_DONE:
      if (timeOutReached(timeout + 2000))
        state = SMA_RECEIVE_POWER;
      break;
    default:
      break;
  }
/*
 // use this to read in a round robin fashion as fast as possible. Tested, OK.
 SMAModbus->tryRead(3,30775,signed32, P183_IPaddress,SMA_Power);
 SMAModbus->tryRead(3,30517,unsigned64, P183_IPaddress,SMA_Energy);
*/
}


SMA::SMA() {
  IP = nullptr;
  ID = 3;
  SMAModbus = new Modbus();
  state = SMA_RECEIVE_POWER;
}
#ifndef CONFIG
#define CONFIG(n) (Settings.TaskDevicePluginConfig[event->TaskIndex][n])
#endif

SMA*  Plugin_183_SMA = NULL;


//==============================================
// PLUGIN
// =============================================

boolean Plugin_183(byte function, struct EventStruct *event, String& string) {
  boolean success = false;

  switch (function)
  {
    case PLUGIN_TEN_PER_SECOND:
      {
        if (Plugin_183_SMA)
          Plugin_183_SMA->handle();
        break;
      }

    case PLUGIN_SERIAL_IN:
      {
        break;
      }
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_183;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;
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
        string = F(PLUGIN_NAME_183);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_183));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_183));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_183));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormTextBox(string, String(F("IP address "))  , String(F("Plugin_183_templateIP")) , P183_IPaddress, 20);
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {

        String argName;
        argName = F("Plugin_183_templateIP");
        strncpy(P183_IPaddress, WebServer.arg(argName).c_str(), sizeof(P183_IPaddress));
        SaveCustomTaskSettings(event->TaskIndex, (byte*)&P183_IPaddress, sizeof(P183_IPaddress));
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&P183_IPaddress, sizeof(P183_IPaddress));
        if (Plugin_183_SMA)
          delete Plugin_183_SMA;
        Plugin_183_SMA = new SMA();
        UserVar[event->BaseVarIndex + 0] = 0;
        UserVar[event->BaseVarIndex + 1] = 0;
        UserVar[event->BaseVarIndex + 2] = 0;
        success = true;
        break;
      }
    case PLUGIN_READ:
      {

        if (!Plugin_183_SMA)
          return success;

        UserVar[event->BaseVarIndex + 0] = Plugin_183_SMA->SMA_Power;
        if (UserVar[event->BaseVarIndex + 0] <0) UserVar[event->BaseVarIndex + 0] = sqrt(-1); 
        UserVar[event->BaseVarIndex + 1] = Plugin_183_SMA->SMA_Energy;
        if (UserVar[event->BaseVarIndex + 1] <0) UserVar[event->BaseVarIndex + 1] = sqrt(-1); 
        UserVar[event->BaseVarIndex + 2] = Plugin_183_SMA->getModbusErrors();

        String log;
        log = F("Power: ");
        log += UserVar[event->BaseVarIndex + 0];
        log += F(" Energy: ");
        log += UserVar[event->BaseVarIndex + 1];
        addLog(LOG_LEVEL_INFO, log);

        success = true;
        break;
      }
  }
  return success;
}

//#endif // testing


