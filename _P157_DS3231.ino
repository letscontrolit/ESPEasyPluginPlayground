/*##########################################################################################
  ############################### Plugin 157: DS3231 RTC ###################################
  ##########################################################################################
  Needs Petre Rodan's ds3231 library to work!
        https://github.com/rodan/ds3231

  Features :
	- Reads date and temperature from DS3231
	- grab&replace essential ESPEasy time functions (not nice from a plugin, but works...
	  but how long?)
	- use ntp at startup then disable immediately (2 time source=nasty problems)
	- can be set the time with "setdate" command

  List of commands :
	- setdate,year,month,day,hour,minute,second
        - setclock,0/1/2     (0=no clock source, 1=ntp enabled, 2=rtc enabled)
        - getalarm           Shows next alarm date

  Command Examples :
	-  /control?cmd=setdate,2018,10,29,14,58,00     Set date to: 2018.10.29 14:58:00
        -  /control?cmd=setclock,1                      Enable NTP-Disable RTC
        -  /control?cmd=setclock,2                      Enable RTC-Disable NTP

  ------------------------------------------------------------------------------------------
	Copyleft Nagy Sándor 2018 - https://bitekmindenhol.blog.hu/
  ------------------------------------------------------------------------------------------
*/

#ifdef PLUGIN_BUILD_TESTING

#include "ds3231.h"
// https://github.com/rodan/ds3231

#define PLUGIN_157
#define PLUGIN_ID_157     157
#define PLUGIN_NAME_157   "RTC - DS3231"
#define PLUGIN_VALUENAME1_157 "Temperature"
#define PLUGIN_VALUENAME2_157 "Tick"        // seconds from 1970.01.01 / unixtime

#define MAX_NTP_RETRIES   10

bool Plugin_157_ntpsync = false;
bool Plugin_157_ntpinit = false;
byte Plugin_157_ntpretries = 0;
unsigned long Plugin_157_interval = 0;
bool Plugin_157_ntpvar = Settings.UseNTP;
bool Plugin_157_init = false;
unsigned long Plugin_157_uptime = 2147483647u; // wdcounter check

boolean Plugin_157(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        //This case defines the device characteristics, edit appropriately

        Device[++deviceCount].Number = PLUGIN_ID_157;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_DUAL;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = false;
        Device[deviceCount].GlobalSyncOption = true;
        Device[deviceCount].DecimalsOnly = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_157);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_157));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_157));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormCheckBox(F("Enable NTP sync at startup"), F("plugin_157_ntp"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);

        int choice = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String alarmOptions[4];
        alarmOptions[0] = F("disabled");
        alarmOptions[1] = F("seconds");
        alarmOptions[2] = F("minutes");
        alarmOptions[3] = F("hours");
        int alarmoptionValues[4] = { 0, 1, 2, 3 };
        addFormSelector(F("Alarm1 interval unit"), F("plugin_157_alarmmode"), 4, alarmOptions, alarmoptionValues, choice);
        addFormNumericBox(F("Alarm1 interval value"), F("plugin_157_alarminterval"), Settings.TaskDevicePluginConfig[event->TaskIndex][2], 0, 5040);
        addFormNote(F("When alarm active it pulls INT/SQW output active Low - requires external pullup"));

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = isFormItemChecked(F("plugin_157_ntp"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_157_alarmmode"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("plugin_157_alarminterval"));
        success = true;
        break;

      }
    case PLUGIN_INIT:
      {

        Plugin_157_ntpsync = Settings.TaskDevicePluginConfig[event->TaskIndex][0];

        String ilogs = F("RTC  :");
        Plugin_157_interval = 0;
        switch (Settings.TaskDevicePluginConfig[event->TaskIndex][1]) {
          case 1:
            Plugin_157_interval = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
            break;
          case 2:
            Plugin_157_interval = (Settings.TaskDevicePluginConfig[event->TaskIndex][2] * 60);
            break;
          case 3:
            Plugin_157_interval = (Settings.TaskDevicePluginConfig[event->TaskIndex][2] * 3600);
            break;
        }
        ilogs = F(" Alarm interval ");
        ilogs += String(Plugin_157_interval);
        ilogs += F(" seconds");

        unsigned long loctime = plugin_157_initialize();

        if ( (loctime > 0) && (year(loctime) > 2000) && (year(loctime) < 3000) ) {
          UserVar[event->BaseVarIndex] = DS3231_get_treg(); // get temperature from rtc, it is free
          UserVar[event->BaseVarIndex + 1] = loctime;
          ilogs += F(" Get init time from RTC. Unix time: ");
          ilogs += String(loctime);
          Plugin_157_init = true;
          success = true;
        } else {
          Plugin_157_init = false;
          success = false;
          ilogs += F(" DS3231 init failed.");
          Settings.UseNTP = Plugin_157_ntpvar;
        }
        addLog(LOG_LEVEL_INFO, ilogs);
        break;
      }

    case PLUGIN_READ:
      {
        if ((Plugin_157_uptime > wdcounter) || (Plugin_157_uptime == 0)) { // check if init did'nt occured already
          Plugin_157_uptime = (wdcounter + 1);
          plugin_157_initialize();
        }

        if (Plugin_157_init) {
          if ((Plugin_157_ntpinit == false) && (Plugin_157_ntpsync) && (Plugin_157_ntpretries < MAX_NTP_RETRIES)) { // check if ntpinit needed
            Plugin_157_ntpretries = Plugin_157_ntpretries + 1;
            plugin_157_do_ntpsync();
            if ((Plugin_157_interval > 0) && (Plugin_157_ntpinit)) { // if alarm enabled and time changed set it up
              DS3231_clear_a1f();
              plugin_157_setnextalarm();
            }
          }
          unsigned long loctime = rtcnow(); // refresh uservars
          if (loctime > 0) {
            UserVar[event->BaseVarIndex] = DS3231_get_treg();
            UserVar[event->BaseVarIndex + 1] = loctime;
          }
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
          struct ts t2;
          t2.year = syear.toInt();
          if ((t2.year < 2018) || (t2.year > 3000)) {
            t2.year = 2018;
          }
          t2.mon = smonth.toInt();
          if ((t2.mon < 1) || (t2.mon > 12)) {
            t2.mon = 1;
          }
          t2.mday = sday.toInt();
          if ((t2.mday < 1) || (t2.mday > 31)) {
            t2.mday = 1;
          }
          t2.hour = shour.toInt();
          if ((t2.hour < 0) || (t2.hour > 23)) {
            t2.hour = 0;
          }
          t2.min = sminute.toInt();
          if ((t2.min < 0) || (t2.min > 59)) {
            t2.min = 0;
          }
          t2.sec = ssecond.toInt();
          if ((t2.sec < 0) || (t2.sec > 59)) {
            t2.sec = 0;
          }
          DS3231_set(t2);
          if (Plugin_157_interval > 0) { // if alarm enabled set it up
            DS3231_clear_a1f();
            plugin_157_setnextalarm();
          }
          nextSyncTime = 0;
          rtcnow();
          command = F("\nOk\nTime updated!");
          SendStatus(event->Source, command);
          success = true;
        }
        if (command == F("setclock")) {
          String sclk = parseString(string, 2);
          byte bclk = (byte)sclk.toInt();
          switch (bclk) {
            case 0:
              Plugin_157_init = false;
              Settings.UseNTP = false;
              break;
            case 1:
              Plugin_157_init = false;
              plugin_157_do_ntpsync();
              Settings.UseNTP = true;
              break;
            case 2:
              Settings.UseNTP = false;
              Plugin_157_init = true;
              nextSyncTime = 0;
              Plugin_157_ntpretries = 0;
              rtcnow();    // get time from RTC
              if (Plugin_157_interval > 0) { // if alarm enabled set it up
                DS3231_clear_a1f();
                plugin_157_setnextalarm();
              }
              break;
          }
          command = F("\nOk");
          SendStatus(event->Source, command);
          success = true;
        }
        if (command == F("getalarm")) {
          char buff[60];
          DS3231_get_a1(&buff[0], 60);
          command = String(buff);
          SendStatus(event->Source, command);
          success = true;
        }
        break;
      }

    case PLUGIN_EXIT:
      {
        Plugin_157_init = false;
        Settings.UseNTP = Plugin_157_ntpvar;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        if (Plugin_157_init) {
          rtccheckTime();                   // handle timer events
          if (Plugin_157_interval > 0) {    // handle triggered alarms and retrigger them
            if (DS3231_triggered_a1()) {
              plugin_157_setnextalarm();
              DS3231_clear_a1f();
              String event = F("RTC#Alarm1");
              rulesProcessing(event);
            }
          }
        }
        success = true;
      }

  }   // switch
  return success;

}     //function

//implement plugin specific procedures and functions here
unsigned long rtcnow() { // ugly hack based on core now()
  // calculate number of seconds passed since last call to rtcnow()
  bool timeSynced = false;
  const long msec_passed = timePassedSince(prevMillis);
  const long seconds_passed = msec_passed / 1000;
  sysTime += seconds_passed;
  prevMillis += seconds_passed * 1000;
  if (nextSyncTime <= sysTime) {
    // nextSyncTime & sysTime are in seconds
    unsigned long  t = getRtcTime();
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

unsigned long getRtcTime() // based on core getNtpTime()
{
  if (Settings.UseNTP) {
    return 0;
  }
  struct ts t;
  DS3231_get(&t);
  if ((t.year < 2000) || (t.year > 3000)) { // rtc read error?
   Plugin_157_init = false; // stop trying?
   String logs = F("RTC time read failed.");
   addLog(LOG_LEVEL_INFO, logs);
   return 0;
  } else
  {
   String logs = F("RTC time read from chip : ");
   logs += t.year;
   logs += F(".");
   logs += t.mon;
   logs += F(".");
   logs += t.mday;
   logs += F(" ");
   logs += t.hour;
   logs += F(":");
   logs += t.min;
   logs += F(":");
   logs += t.sec;
   addLog(LOG_LEVEL_INFO, logs);
   timeStruct tm2;
   tm2.Year = t.year - 1970;
   tm2.Month = t.mon;
   tm2.Day = t.mday;
   tm2.Hour = t.hour;
   tm2.Minute = t.min;
   tm2.Second = t.sec;
   return makeTime(tm2); // return unix time
  }
}

void rtccheckTime()
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

void plugin_157_setnextalarm(void) // setup next alarm
{
  unsigned long timenow = getUnixTime();  // based on current unix time
  struct timeStruct tma;
  breakTime((timenow + Plugin_157_interval), tma);  // and interval
  uint8_t flags[5] = { 0, 0, 0, 0, 0 };
  String logs = F("RTC  : Next alarm time set to: ");
  logs += String(tma.Hour) + F(":") + String(tma.Minute) + F(":") + String(tma.Second);
  addLog(LOG_LEVEL_INFO, logs);
  DS3231_set_a1(tma.Second, tma.Minute, tma.Hour, tma.Day, flags);
  DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A1IE);    // activate Alarm1
}

uint32_t plugin_157_deLocal(uint32_t utc)
{
  if (year(utc) != year(m_dstUTC)) calcTimeChanges(year(utc));

  if (utcIsDST(utc))
    return utc - (m_dst.offset * SECS_PER_MIN);
  else
    return utc - (m_std.offset * SECS_PER_MIN);
}

void plugin_157_do_ntpsync()
{
  Settings.UseNTP = true;
  Plugin_157_ntpinit = false;
  nextSyncTime = 0;
  String logs;
  unsigned long t = getNtpTime(); // get time from ntp if available
  if (t != 0) {
    //t = now();
    struct ts t2;
    breakTime(toLocal(t), tm); // timezone is a major headache...
    t2.sec = tm.Second;
    t2.min = tm.Minute;
    t2.hour = tm.Hour;
    t2.mday = tm.Day;
    t2.wday = tm.Wday;
    t2.mon = tm.Month;
    t2.year = (tm.Year + 1970);
    String logs = F("RTC  : Time from NTP: ");
    logs += t2.year;
    logs += F(".");
    logs += t2.mon;
    logs += F(".");
    logs += t2.mday;
    logs += F(" ");
    logs += t2.hour;
    logs += F(":");
    logs += t2.min;
    logs += F(":");
    logs += t2.sec;
    addLog(LOG_LEVEL_INFO, logs);
    if (t2.year > 2017) {       // if time seems valid update RTC
      DS3231_set(t2);
      logs = F("RTC  : Updating time from NTP. Unix time: ");
      logs += String(getUnixTime());
      addLog(LOG_LEVEL_INFO, logs);
      Plugin_157_ntpinit = true;
    }
  } else {
    logs = F("RTC  : NTP sync failed.");
    addLog(LOG_LEVEL_INFO, logs);
  }
  Settings.UseNTP = false;
}

unsigned long plugin_157_initialize()
{
  Plugin_157_ntpretries = 0;

  Plugin_157_ntpvar = Settings.UseNTP; // save ntp state
  if (Plugin_157_ntpsync) { // if ntp sync enabled
    plugin_157_do_ntpsync();
  }
  DS3231_init(DS3231_CONTROL_INTCN);   // init ds3231, do not use wire.begin it is handled by espeasy
  nextSyncTime = 0;
  syncInterval = 1800;
  unsigned long loctime = rtcnow();    // get time from RTC
  if (Plugin_157_interval > 0) { // if alarm enabled set it up
    DS3231_clear_a1f();
    plugin_157_setnextalarm();
  }
  return loctime;
}
#endif
