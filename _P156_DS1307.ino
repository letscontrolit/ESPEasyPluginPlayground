/*##########################################################################################
  ############################### Plugin 156: DS1307 RTC ###################################
  ##########################################################################################

  Features :
	- Reads date from DS1307
	- grab&replace essential ESPEasy time functions (not nice from a plugin, but works...
	  but how long?)
	- use ntp at startup then disable immediately (2 time source=nasty problems)
	- can be set the time with "setdate" command
        - Do not use DS3231 and DS1307 plugin at the same time, there are absolutely no sense

  List of commands :
	- setdate,year,month,day,hour,minute,second
        - setclock,0/1/2     (0=no clock source, 1=ntp enabled, 2=rtc enabled)

  Command Examples :
	-  /control?cmd=setdate,2018,10,29,14,58,00     Set date to: 2018.10.29 14:58:00
        -  /control?cmd=setclock,1                      Enable NTP-Disable RTC
        -  /control?cmd=setclock,2                      Enable RTC-Disable NTP

  ------------------------------------------------------------------------------------------
	Copyleft Nagy Sándor 2018 - https://bitekmindenhol.blog.hu/
  ------------------------------------------------------------------------------------------
*/

#ifdef PLUGIN_BUILD_TESTING

#define PLUGIN_156
#define PLUGIN_ID_156     156
#define PLUGIN_NAME_156   "RTC - DS1307"
#define PLUGIN_VALUENAME1_156 "Tick"        // seconds from 1970.01.01 / unixtime

#define MAX_NTP_RETRIES   10
#define DS1307_CTRL_ID 0x68

bool Plugin_156_ntpsync = false;
bool Plugin_156_ntpinit = false;
byte Plugin_156_ntpretries = 0;
unsigned long Plugin_156_interval = 0;
bool Plugin_156_ntpvar = Settings.UseNTP;
bool Plugin_156_init = false;
unsigned long Plugin_156_uptime = 2147483647u; // wdcounter check

boolean Plugin_156(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        //This case defines the device characteristics, edit appropriately

        Device[++deviceCount].Number = PLUGIN_ID_156;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_DUAL;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = false;
        Device[deviceCount].GlobalSyncOption = true;
        Device[deviceCount].DecimalsOnly = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_156);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_156));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormCheckBox(F("Enable NTP sync at startup"), F("plugin_156_ntp"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = isFormItemChecked(F("plugin_156_ntp"));
        success = true;
        break;

      }
    case PLUGIN_INIT:
      {
        String ilogs;
        Plugin_156_ntpsync = Settings.TaskDevicePluginConfig[event->TaskIndex][0];

        unsigned long loctime = plugin_156_initialize();

        if ( (loctime > 0) && (year(loctime) > 2000) && (year(loctime) < 3000) ) {
          UserVar[event->BaseVarIndex] = loctime;
          ilogs += F(" Get init time from RTC. Unix time: ");
          ilogs += String(loctime);
          Plugin_156_init = true;
          success = true;
        } else {
          Plugin_156_init = false;
          success = false;
          ilogs += F(" DS1307 init failed.");
          Settings.UseNTP = Plugin_156_ntpvar;
        }
        addLog(LOG_LEVEL_INFO, ilogs);
        break;
      }

    case PLUGIN_READ:
      {
        if ((Plugin_156_uptime > wdcounter) || (Plugin_156_uptime == 0)) { // check if init did'nt occured already
          Plugin_156_uptime = (wdcounter + 1);
          plugin_156_initialize();
        }

        if (Plugin_156_init) {
          if ((Plugin_156_ntpinit == false) && (Plugin_156_ntpsync) && (Plugin_156_ntpretries < MAX_NTP_RETRIES)) { // check if ntpinit needed
            Plugin_156_ntpretries = Plugin_156_ntpretries + 1;
            plugin_156_do_ntpsync();
          }
          unsigned long loctime = plugin_156_rtcnow(); // refresh uservars
          UserVar[event->BaseVarIndex] = loctime;
        }
        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);
        if (command == F("setdate")) {
          String syear = parseString(string, 2);
          String smonth = parseString(string, 3);
          String sday = parseString(string, 4);
          String shour = parseString(string, 5);
          String sminute = parseString(string, 6);
          String ssecond = parseString(string, 7);
          int year = syear.toInt();
          if ((year < 2018) || (year > 3000)) {
            tm.Year = 48;
          } else { tm.Year = year-1970;}
          tm.Month = smonth.toInt();
          if ((tm.Month < 1) || (tm.Month > 12)) {
            tm.Month = 1;
          }
          tm.Day = sday.toInt();
          if ((tm.Day < 1) || (tm.Day > 31)) {
            tm.Day = 1;
          }
          tm.Hour = shour.toInt();
          if ((tm.Hour < 0) || (tm.Hour > 23)) {
            tm.Hour = 0;
          }
          tm.Minute = sminute.toInt();
          if ((tm.Minute < 0) || (tm.Minute > 59)) {
            tm.Minute = 0;
          }
          tm.Second = ssecond.toInt();
          if ((tm.Second < 0) || (tm.Second > 59)) {
            tm.Second = 0;
          }
          DS1307_set(tm);
          nextSyncTime = 0;
          plugin_156_rtcnow();
          command = F("\nOk\nTime updated!");
          SendStatus(event->Source, command);
          success = true;
        }
        if (command == F("setclock")) {
          String sclk = parseString(string, 2);
          byte bclk = (byte)sclk.toInt();
          switch (bclk) {
            case 0:
              Plugin_156_init = false;
              Settings.UseNTP = false;
              break;
            case 1:
              Plugin_156_init = false;
              plugin_156_do_ntpsync();
              Settings.UseNTP = true;
              break;
            case 2:
              Settings.UseNTP = false;
              Plugin_156_init = true;
              nextSyncTime = 0;
              Plugin_156_ntpretries = 0;
              plugin_156_rtcnow();    // get time from RTC
              break;
          }
          command = F("\nOk");
          SendStatus(event->Source, command);
          success = true;
        }
        break;
      }

    case PLUGIN_EXIT:
      {
        Plugin_156_init = false;
        Settings.UseNTP = Plugin_156_ntpvar;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        if (Plugin_156_init) {
          plugin_156_rtccheckTime();                   // handle timer events
        }
        success = true;
      }

  }   // switch
  return success;

}     //function

//implement plugin specific procedures and functions here
unsigned long plugin_156_rtcnow() { // ugly hack based on core now()
  // calculate number of seconds passed since last call to rtcnow()
  bool timeSynced = false;
  const long msec_passed = timePassedSince(prevMillis);
  const long seconds_passed = msec_passed / 1000;
  sysTime += seconds_passed;
  prevMillis += seconds_passed * 1000;
  if (nextSyncTime <= sysTime) {
    // nextSyncTime & sysTime are in seconds
    unsigned long  t = plugin_156_getRtcTime();
    if (t != 0) {
      timeSynced = true;
      setTime(t);
    }
  }
  //uint32_t localSystime = toLocal(sysTime);
  //breakTime(localSystime, tm);
  breakTime(sysTime, tm);
  if (timeSynced) {
    calcSunRiseAndSet();
  }
  //return (unsigned long)localSystime;
  return (unsigned long)sysTime;
}

unsigned long plugin_156_getRtcTime() // based on core getNtpTime()
{
  if (Settings.UseNTP) {
    return 0;
  }
  DS1307_get(tm);
  if (tm.Year < 31) { // rtc read error?
    nextSyncTime = sysTime + 60;
  }
  String logs = F("RTC time read from chip : ");
  logs += (tm.Year+1970);
  logs += F(".");
  logs += tm.Month;
  logs += F(".");
  logs += tm.Day;
  logs += F(" ");
  logs += tm.Hour;
  logs += F(":");
  logs += tm.Minute;
  logs += F(":");
  logs += tm.Second;
  addLog(LOG_LEVEL_INFO, logs);
  return makeTime(tm); // return unix time
}

void plugin_156_rtccheckTime()
{
  if (Settings.UseNTP) {
    return;
  }
  rtcnow();
  if (tm.Minute != PrevMinutes)
  {
    PluginCall(PLUGIN_CLOCK_IN, 0, dummyString);
    PrevMinutes = tm.Minute;
    if (Settings.UseRules)
    {
      String event;
      event.reserve(21);
      event = F("Clock#Time=");
      event += weekday_str();
      event += ",";
      if (hour() < 10)
        event += "0";
      event += hour();
      event += ":";
      if (minute() < 10)
        event += "0";
      event += minute();
      rulesProcessing(event);
    }
  }
}

void plugin_156_do_ntpsync()
{
  Settings.UseNTP = true;
  Plugin_156_ntpinit = false;
  nextSyncTime = 0;
  String logs;
  unsigned long t = getNtpTime(); // get time from ntp if available
  if (t != 0) {
    //t = now();
    breakTime(toLocal(t), tm); // timezone is a major headache...
    String logs = F("RTC  : Time from NTP: ");
  logs += (tm.Year+1970);
  logs += F(".");
  logs += tm.Month;
  logs += F(".");
  logs += tm.Day;
  logs += F(" ");
  logs += tm.Hour;
  logs += F(":");
  logs += tm.Minute;
  logs += F(":");
  logs += tm.Second;
    addLog(LOG_LEVEL_INFO, logs);
    if (tm.Year > 47) {       // if time seems valid update RTC
      DS1307_set(tm);
      logs = F("RTC  : Updating time from NTP. Unix time: ");
      logs += String(getUnixTime());
      addLog(LOG_LEVEL_INFO, logs);
      Plugin_156_ntpinit = true;
    }
  } else {
    logs = F("RTC  : NTP sync failed.");
    addLog(LOG_LEVEL_INFO, logs);
  }
  Settings.UseNTP = false;
}

unsigned long plugin_156_initialize()
{
  Plugin_156_ntpretries = 0;

  Plugin_156_ntpvar = Settings.UseNTP; // save ntp state
  if (Plugin_156_ntpsync) { // if ntp sync enabled
    plugin_156_do_ntpsync();
  }
  nextSyncTime = 0;
  syncInterval = 1800;
  unsigned long loctime = plugin_156_rtcnow();    // get time from RTC
  return loctime;
}

bool DS1307_get(struct timeStruct &tml)
{
  Wire.beginTransmission(DS1307_CTRL_ID);
  Wire.write((byte)0x00);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_CTRL_ID, 7);
  if (Wire.available() < 7) {
    return false;
  }

  uint8_t sec = Wire.read();
  tml.Second = bcd2bin(sec & 0x7f);
  tml.Minute = bcd2bin(Wire.read() );
  tml.Hour =   bcd2bin(Wire.read() & 0x3f);
  tml.Wday = bcd2bin(Wire.read() );
  tml.Day = bcd2bin(Wire.read() );
  tml.Month = bcd2bin(Wire.read() );
  tml.Year = ((bcd2bin(Wire.read()))+30);
  if (sec & 0x80) {
    return false;
  }
  return true;
}

bool DS1307_set(timeStruct tml)
{
  Wire.beginTransmission(DS1307_CTRL_ID);
  Wire.write((byte)0x00); // reset register pointer
  Wire.write(bin2bcd(tml.Second)); //
  Wire.write(bin2bcd(tml.Minute));
  Wire.write(bin2bcd(tml.Hour));      // sets 24 hour format
  Wire.write(bin2bcd(tml.Wday));
  Wire.write(bin2bcd(tml.Day));
  Wire.write(bin2bcd(tml.Month));
  Wire.write(bin2bcd( (tml.Year-30)));
  Wire.endTransmission();
  if (Wire.endTransmission() != 0) {
    return false;
  }
  return true;
}

uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

#endif
