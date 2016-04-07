//#######################################################################################################
//#################################### Plugin 209: IFTTT Maker###########################################
//#######################################################################################################

#define PLUGIN_209
#define PLUGIN_ID_209        209
#define PLUGIN_NAME_209      "IFTTT Maker"


boolean Plugin_209(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  static byte LimitIndicator[4];				// Keeps track of 0=In Range, 1=Below LL, 2=Above UL

  //	Define the structure for the parameters

  struct TemplateStrucure
  {
	  char Ident[24];
	  char LL[8];
	  char UL[8];
	  char Hysteresis[8];
  } Template[6];

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_209;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = true;
        break;
      }
      
    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_209);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
		break;
      }
      
    case PLUGIN_WEBFORM_LOAD:
      {

		LoadCustomTaskSettings(event->TaskIndex, (byte*)&Template, sizeof(Template));

		//	The first two records of the template are used for API and Event

		string += F("<TR><TD>IFTTT Key:<TD> <input type='text' size='40' maxlength='24' name='APIKey' value='");
		string += Template[0].Ident;
		string += "'>";

		string += F("<TR><TD>IFTTT Event:<TD> <input type='text' size='40' maxlength='24' name='Event' value='");
		string += Template[1].Ident;
		string += "'>";

//		Now create table headers

		string+=F("<TR><table id='t01' style='width:50%'>");

		string += F("<thead><tr><th>Identifier<th>Lower Limit<th>Upper Limit<th>Hysteresis</thead>");

//		And display the table - 4 rows

		for (byte varNr = 0; varNr < 4; varNr++)
		{

			string += F("<TR><TD><input type='text' size='40' maxlength='24' name='Ident_");
			string += varNr+1;
			string += F("' value ='");
			string += Template[varNr+2].Ident;	
			string += F("'</TD><TD><input type='text' size='10' maxlength='8' name='LL_");
			string += varNr+1;
			string += F("' value ='");
			string += Template[varNr+2].LL;			
			string += F("'</TD><TD><input type='text' size='10' maxlength='8' name='UL_");
			string += varNr+1;
			string += F("' value ='");
			string += Template[varNr+2].UL;	
			string += F("'</TD><TD><input type='text' size='10' maxlength='8' name='Hys_");
			string += varNr + 1;
			string += F("' value ='");
			string += Template[varNr+2].Hysteresis;
			string += F("'/TD>");
		}

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {

		strncpy(Template[0].Ident, WebServer.arg("APIKey").c_str(), 24);
		strncpy(Template[1].Ident, WebServer.arg("Event").c_str(), 24);

		String argName;

		for (byte varNr = 0; varNr < 4; varNr++)
		{
			argName = F("Ident_");
			argName += varNr + 1;
			strncpy(Template[varNr+2].Ident, WebServer.arg(argName).c_str(), 24);

			argName = F("LL_");
			argName += varNr + 1;
			strncpy(Template[varNr + 2].LL, WebServer.arg(argName).c_str(), 8);

			argName = F("UL_");
			argName += varNr + 1;
			strncpy(Template[varNr + 2].UL, WebServer.arg(argName).c_str(), 8);

			argName = F("Hys_");
			argName += varNr + 1;
			strncpy(Template[varNr + 2].Hysteresis, WebServer.arg(argName).c_str(), 8);

		}

		//Settings.TaskDeviceID[event->TaskIndex] = 1; // temp fix, needs a dummy value

		SaveCustomTaskSettings(event->TaskIndex, (byte*)&Template, sizeof(Template));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
		  //	Initialize the limit indicators

		  for (byte x = 0; x < 4; x++)
		  {
			  LimitIndicator[x] = 0;
		  }

		success = true;
        break;
      }

    case PLUGIN_READ:
      {
//		Here we loop over all four idents and get the latest values

		LoadCustomTaskSettings(event->TaskIndex, (byte*)&Template, sizeof(Template));

		String Ident;
		String log;
		float UL;
		float LL;
		float Hysteresis;

		for (byte x = 0; x < 4; x++)
		{
			Ident = Template[x + 2].Ident;
			Ident.trim();
			if (Ident.length() == 0)break;

			float LV = getLatestValue(Ident);
			if (LV == -999.00) break;

			LL = string2float(Template[x+2].LL);
			if (LL == -999) {
				String log = F("IFTTT: Illegal Value for Lower Limit ");
				log += Template[x].LL;
				addLog(LOG_LEVEL_ERROR, log);
				break;
			}

			UL = string2float(Template[x+2].UL);
			if (UL == -999) {
				String log = F("IFTTT: Illegal Value for Upper Limit ");
				log += Template[x+2].UL;
				addLog(LOG_LEVEL_ERROR, log);
				break;
			}

			if (LL >= UL) {
				String log = F("IFTTT: Lower Limit must be less than Upper Limit");
				addLog(LOG_LEVEL_ERROR, log);
				break;
			}

			Hysteresis = string2float(Template[x + 2].Hysteresis);
			if (Hysteresis < 0) {
				String log = F("IFTTT: Illegal Value for Hysteresis ");
				log += Template[x + 2].Hysteresis;
				addLog(LOG_LEVEL_ERROR, log);
				break;
			}

			// IFTTT uses 'Event' to identify a particular trigger - this must match a parameter on IFTTT.com

			String IFTTT_APIKey = Template[0].Ident;
			String IFTTT_Event = Template[1].Ident;

			// IFTTT allows only 3 data values to be sent along with the trigger
			// We define these as follows:

			String IFTTT_Value1 = Ident;
			String IFTTT_Value2 = String(LV);		// This is the current value
			String IFTTT_Value3 = "[" + String(LL) + "/" + String(UL) + "]";	// The min/max combo

//	All input data is now checked - check the data levels

			if (LimitIndicator[x] == 0) {
				if(LV > UL) {
					LimitIndicator[x] = 2;
					log = "IFTTT: ";
					log += Ident;
					log += " has a value of ";
					log += LV;
					log += " which is above the upper limit - IFTTT Trigger will be sent";
					addLog(LOG_LEVEL_INFO, log);
					IFTTT_Trigger(IFTTT_APIKey, IFTTT_Event, IFTTT_Value1,IFTTT_Value2,IFTTT_Value3);
					break;
				}
				else if (LV < LL) {
					LimitIndicator[x] = 1;
					log = "IFTTT: ";
					log += Ident;
					log += " has a value of ";
					log += LV;
					log += " which is below the lower limit - IFTTT Trigger will be sent";
					addLog(LOG_LEVEL_INFO, log);
					IFTTT_Trigger(IFTTT_APIKey, IFTTT_Event, IFTTT_Value1, IFTTT_Value2, IFTTT_Value3);
					break;
				}
			}
			else if (LimitIndicator[x] == 2) {
				if (LV < UL - Hysteresis) {
					LimitIndicator[x] = 0;
					log = "IFTTT: ";
					log += Ident;
					log += " has a value of ";
					log += LV;
					log += " - hysteresis reset";
					addLog(LOG_LEVEL_INFO, log);
					break;
				}
			}
			else if (LimitIndicator[x] == 1) {
					if (LV > LL + Hysteresis) {
						LimitIndicator[x] = 0;
						log = "IFTTT: ";
						log += Ident;
						log += " has a value of ";
						log += LV;
						log += " - hysteresis reset";
						addLog(LOG_LEVEL_INFO, log);
						break;
					}
			}
		}

        success = true;
        break;
      }

  }
  return success;
}
