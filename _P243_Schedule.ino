#ifdef USES_P243
//#######################################################################################################
//#################################### Plugin 243: Schedule #########################################
//#######################################################################################################
#define PLUGIN_243
#define PLUGIN_ID_243         243
#define PLUGIN_NAME_243       "Generic - Schedule [TESTING]"
#define PLUGIN_VALUENAME1_243 "Running"
#define PLUGIN_VALUENAME2_243 "RunValue"
#define PLUGIN_VALUENAME3_243 "Override"
#define PLUGIN_243_MAX_SETTINGS 4
#define PLUGIN_243_PERIODS_COUNT 4 
#define P243_PERIOD 					PCONFIG(2)
#define P243_PERIOD_ONCE			0
#define P243_PERIOD_DAILY			1
#define P243_PERIOD_WEEKLY		2
#define P243_PERIOD_MONTHLY		3
//#define P243_DAYS							*((uint32_t *) &(Settings.TaskDevicePluginConfig[event->TaskIndex][3]))
#define P243_DT_START(n)			PCONFIG_LONG(n)
#define P243_DT_END(n)				ExtraTaskSettings.TaskDevicePluginConfigLong[n]
#define P243_SETVALUE(n)			PCONFIG_FLOAT(n)
#define P243_VAR_RUNNING			UserVar[event->BaseVarIndex]
#define P243_VAR_RUNVALUE			UserVar[event->BaseVarIndex+1]
#define P243_VAR_OVERRIDE			UserVar[event->BaseVarIndex+2]

typedef void (*AddSetValueT) (byte x, float v);

bool *LoadClockSet(int16_t period, bool days[], struct EventStruct *event);

boolean Plugin_243(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_243;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_NONE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].TimerOptional = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_243);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_243));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_243));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_243));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {

				addHtml(F("<TR><TD>Task:<TD>"));
				addTaskSelect(F("p243_task"), PCONFIG(0));

				LoadTaskSettings(PCONFIG(0)); // we need to load the values from another task for selection!
				addHtml(F("<TR><TD>Set Value:<TD>"));
				addTaskValueSelect(F("p243_value"), PCONFIG(1), PCONFIG(0));

				AddSetValueT addSetValue = &addSetValueFloat;

				switch (Device[getDeviceIndex(Settings.TaskDeviceNumber[PCONFIG(0)])].VType)
				{
					case SENSOR_TYPE_SWITCH:
						addSetValue = addSetValueSwitch;
						break;
				
					default:
						break;
				}
				LoadTaskSettings(event->TaskIndex); // restore original taskvalues

				String periods[PLUGIN_243_PERIODS_COUNT];
				periods[0] = "Once";
				periods[1] = "Daily";
				periods[2] = "Weekly";
				periods[3] = "Monthly";

				addFormSelector(F("Period"), F("p243_period"), PLUGIN_243_PERIODS_COUNT, periods, NULL, P243_PERIOD, true);
				
				if ((P243_PERIOD == P243_PERIOD_WEEKLY) || (P243_PERIOD == P243_PERIOD_MONTHLY))
				{
					bool days[31];
					LoadClockSet(P243_PERIOD, days, event);
					addHtml(F("<TR><TD colspan=\"7\"><TABLE><TR>"));
					for (byte x = 0; x < GetDaysForPeriod(P243_PERIOD); x++)
					{
						if (((x % 8) == 0) & (x /8 > 0)) addHtml(F("</TR><TR>"));
						addHtml(F("<TD>"));
						
						addHtml(String(x+1));
						addCheckBox(String(F("p243_day")) + (x),days[x]);
						addHtml(F("</TD>"));
					}
					addHtml(F("</TR></TABLE></TD></TR>"));
				}

        for (byte x = 0; x < PLUGIN_243_MAX_SETTINGS; x++)
        {
        	addFormTextBox(String(F("Time ")) + (x + 1),
						String(F("p243_clock_start")) + (x), 
						DateTimeintString(P243_PERIOD, P243_DT_START(x)),
						20);
					addHtml(" ");
					addTextBox(String(F("p243_clock_end")) + (x),
						DateTimeintString(P243_PERIOD, P243_DT_END(x)),
						20);
					addHtml(" ");
					addSetValue(x, P243_SETVALUE(x));
        }
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
				PCONFIG(0) = getFormItemInt(F("p243_task"));
				PCONFIG(1) = getFormItemInt(F("p243_value"));

				for (byte x = 0; x < PLUGIN_243_MAX_SETTINGS; x++)
				{
					P243_DT_START(x) = String2Timeint(P243_PERIOD, WebServer.arg(String(F("p243_clock_start")) + (x)));
					P243_DT_END(x) = String2Timeint(P243_PERIOD, WebServer.arg(String(F("p243_clock_end")) + (x)));
					P243_SETVALUE(x) = getFormItemFloat(String(F("p243_setvalue")) + (x));
				}

				bool days[31];
				for (byte x = 0; x < GetDaysForPeriod(P243_PERIOD); x++)
				{
					days[x] = isFormItemChecked(String(F("p243_day")) + (x));
				}
				SaveClockSet(P243_PERIOD, days, event);

				P243_PERIOD = getFormItemInt(F("p243_period"));

				success = true;
				break;
      }

    case PLUGIN_INIT:
      {
        success = true;
        break;
      }

    case PLUGIN_CLOCK_IN:
      {
        LoadTaskSettings(event->TaskIndex);

				if (!P243_VAR_OVERRIDE) {
				bool isDay;
				if ((P243_PERIOD == P243_PERIOD_ONCE) | (P243_PERIOD == P243_PERIOD_DAILY))
					isDay = true;
				else
				{
					bool days[31];
					LoadClockSet(P243_PERIOD, days, event);
					isDay = ((P243_PERIOD == P243_PERIOD_WEEKLY) & days[weekday()-1]) |
									((P243_PERIOD == P243_PERIOD_MONTHLY) & days[day()-1]);
				}

					P243_VAR_RUNNING = 0;
				if (isDay)
				{	 
					time_t curtime = now();
					// if not an exact date - get time of a day
					if (P243_PERIOD != P243_PERIOD_ONCE) 
					{
						curtime %= 60*60*24;
					}

					for (byte x = 0; x < PLUGIN_243_MAX_SETTINGS; x++)
					{
						time_t dt_end = P243_DT_END(x);
						// for ability setting TimeEnd on the next day
						if (dt_end == 0)
						{
							dt_end = 60*60*24;
						}

						if ((curtime >= P243_DT_START(x)) && (curtime < dt_end))
						{
								String log = F("SCHED: Running at period ");
        	  	log += x;
								log += F(" with value ");
							log += P243_SETVALUE(x);
	          	addLog(LOG_LEVEL_INFO, log);
								P243_VAR_RUNNING = 1;
								P243_VAR_RUNVALUE = P243_SETVALUE(x);
							break;
						}
					}
				}
				}

				if (P243_VAR_RUNNING)
				{
					UserVar[PCONFIG(0) * VARS_PER_TASK + PCONFIG(1)] = P243_VAR_RUNVALUE;
					sendData(event);
				}
				success = true;
        break;
      }
  }
  return success;
}

int16_t GetDaysForPeriod(int16_t period)
{
	switch (period)
	{
		case P243_PERIOD_WEEKLY:
			return 7;
			break;
		case P243_PERIOD_MONTHLY:
			return 31;
			break;
		default:
			return 0;
			break;
	}
}

void SaveClockSet(int16_t period, bool days[], struct EventStruct *event)
{
	uint32_t daysPart = 0;
	for (byte x = 0; x < GetDaysForPeriod(period); x++)
	{
		if (days[x]) daysPart |= 1 << x;
	}
	PCONFIG(3) = (int16_t) (daysPart >> 16);
	PCONFIG(4) = (int16_t) (daysPart & 0xFFFF);
}

bool *LoadClockSet(int16_t period, bool days[], struct EventStruct *event)
{
	uint32_t daysPart = ( ( (uint32_t) PCONFIG(3) ) << 16) | (uint32_t) PCONFIG(4);
	for (byte x = 0; x < GetDaysForPeriod(period); x++)
	{
		if (daysPart & (1 << x)) days[x] = true;
		else days[x] = false;
	}

	return days;
}

long String2Timeint(int16_t period, String Timestr)
{
	struct tm tm;

	int yr, mnth, d, h, m, s;

	switch (period)
	{
		case P243_PERIOD_ONCE: 
		{
			sscanf(Timestr.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &yr, &mnth, &d, &h, &m, &s);
			tm.tm_year = yr - 1970;
			tm.tm_mon = mnth;
			tm.tm_mday = d;
			break;
		}
		case P243_PERIOD_DAILY:
		case P243_PERIOD_WEEKLY:
		case P243_PERIOD_MONTHLY: 
		default:
		{
			sscanf(Timestr.c_str(), "%2d:%2d:%2d", &h, &m, &s);
			tm.tm_year = 0;
			tm.tm_mon = 1;
			tm.tm_mday = 1;
			break;
		}
	}
	tm.tm_hour = h;
	tm.tm_min = m;
	tm.tm_sec = s;

	return makeTime(tm);
}

String DateTimeintString(int16_t period, long Timeint)
{
  struct tm tm;
  breakTime(Timeint, tm);

	String result;

	switch (period)
	{
		case P243_PERIOD_ONCE:
		{
			result = getDateTimeString(tm);
			break;
		}
		case P243_PERIOD_DAILY:
		case P243_PERIOD_WEEKLY:
		case P243_PERIOD_MONTHLY: 
		default:
		{
			result = getTimeString(tm, ':', false, true);
			break;
		}
	}

	return result;
}

void addSetValueFloat(byte x, float v) 
{
	addTextBox(String(F("p243_setvalue")) + (x),
						String(v),
						10);
}

void addSetValueSwitch(byte x, float v) 
{
	String options[2] = { F("0"), F("1") };
	addSelector(String(F("p243_setvalue")) + (x),
		2, 
		options, 
		NULL, 
		NULL, 
		(int)v, 
		false, true);
}
#endif // USES_P243
