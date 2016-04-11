//#######################################################################################################
//#################################### Plugin 209: IFTTT Maker###########################################
//#################################### Version 0.3 12-May-2016 ##########################################
//#######################################################################################################

#define PLUGIN_209
#define PLUGIN_ID_209        209
#define PLUGIN_NAME_209      "IFTTT Maker"

// The line below defines the dummy function PLUGIN_COMMAND which is only for Namirda use

#ifndef PLUGIN_COMMAND
#define PLUGIN_COMMAND 999
#endif   

boolean Plugin_209(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  //	Define the structure to load/save the parameters

  struct TemplateStructure
  {
	  char Ident[24];
	  char LL[8];
	  char UL[8];
	  char Hysteresis[8];
  };
 
 static TemplateStructure Template[6];		// Needs to be static because we use the plugin_init values in plugin_read

  //	Define and Instantiate structure for parameters

  struct DStruct
  {
	  int uservarindex;			// Index in uservar of the latest value
	  float UL;					// Upper limit
	  float LL;					// Lower limit	
	  float Hysteresis;			// Hysteresis
	  byte LimitIndicator;		// Keeps track of 0=In Range, 1=Below LL, 2=Above UL
  };
  
  static DStruct InData[4];				//Needs to be static because we use the plugin_init values in plugin_read

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
		
		  LoadCustomTaskSettings(event->TaskIndex, (byte*)&Template, sizeof(Template));

		  String TaskName;
		  String ValueName;
		  String Ident;
		  int ValueIndex;
		  int TaskIndex;

		  // Preset Index to -1 to skip blank idents during read

		  for (byte x = 0; x < 4; x++) 
		  {
			  InData[x].uservarindex = -1;	//Skip this ident during read unless updated to positive
		  }

		  // Loop over the four parameter sets and store values

		  for (byte x = 0; x < 4; x++)
		  {

			  Ident= Template[x + 2].Ident;
			  Ident.trim();

			  if (Ident.length() == 0)continue;			// Ignore blank idents

			  // Check Ident syntax
			  if (!getTaskandValueName(Ident, TaskName, ValueName)) 
			  {
				  String log = F("IFTTT: Illegal Identifier Syntax - ");
				  log += Ident;
				  log += " - this entry will be ignored";
				  addLog(LOG_LEVEL_ERROR, log);
				  continue;	
			  }

			  // Get and check the TaskIndex
			  TaskIndex = getTaskIndex(TaskName);		
			  if (TaskIndex == 255) {
				  String log = F("IFTTT: Task ");
				  log += TaskName;
				  log += " not found - this entry will be ignored";
				  addLog(LOG_LEVEL_ERROR, log);
				  continue;
			  }

			  //Get and Check the ValueIndex
			  ValueIndex = getValueNameIndex(TaskIndex, ValueName);		
			  if (ValueIndex == 255) {
				  String log = F("IFTTT: Valuename ");
				  log += ValueName;
				  log += " not found for task ";
				  log += TaskName;
				  log += " - this entry will be ignored";
				  addLog(LOG_LEVEL_ERROR, log);
				  continue;
			  }

			  // Check on UL,LL and Hysteresis
			  InData[x].LL = string2float(Template[x + 2].LL);
			  if (InData[x].LL == -999) {
				  String log = F("IFTTT: Illegal Value for Lower Limit ");
				  log += Template[x].LL;
				  addLog(LOG_LEVEL_ERROR, log);
				  continue;
			  }

			  //Check upper limit
			  InData[x].UL = string2float(Template[x + 2].UL);
			  if (InData[x].UL == -999) {
				  String log = F("IFTTT: Illegal Value for Upper Limit ");
				  log += Template[x + 2].UL;
				  addLog(LOG_LEVEL_ERROR, log);
				  continue;
			  }

			  //Check lower limit
			  if (InData[x].LL >= InData[x].UL) {
				  String log = F("IFTTT: Lower Limit must be less than Upper Limit");
				  addLog(LOG_LEVEL_ERROR, log);
				  continue;
			  }

			  //Check Hysteresis
			  InData[x].Hysteresis = string2float(Template[x + 2].Hysteresis);
			  if (InData[x].Hysteresis < 0) {
				  String log = F("IFTTT: Illegal Value for Hysteresis ");
				  log += Template[x + 2].Hysteresis;
				  addLog(LOG_LEVEL_ERROR, log);
				  continue;
			  }

			  // At this point the input parameters looks OK - we can now set uservarindex

			  InData[x].uservarindex = TaskIndex*VARS_PER_TASK + ValueIndex;

		  //	Finally initialize the limit indicators

			  InData[x].LimitIndicator = 0;
		  }

		success = true;
        break;
      }

    case PLUGIN_READ:
      {
		//		Here we loop over all four idents and get the latest values

		float LV;
		
		for (byte x = 0; x < 4; x++)
		{
			if (InData[x].uservarindex == -1)continue; 					// Only bother with valid idents

			LV = UserVar[InData[x].uservarindex];						// This is the latest value for the ident

//	Now check the data levels against limits

			if (InData[x].LimitIndicator == 0) {

				// Check Upper Limit

				if(LV > InData[x].UL) {

					// We are going to send Trigger - prepare stuff

					String IFTTT_APIKey = Template[0].Ident;
					String IFTTT_Event = Template[1].Ident;

					String Ident = Template[x + 2].Ident;
					Ident.trim();

					// Prepare the 3 values to be sent to IFTTT

					String IFTTT_Value1 = Ident;
					String IFTTT_Value2 = String(LV);		// This is the current value
					String IFTTT_Value3 = "[" + String(InData[x].LL) + "/" + String(InData[x].UL) + "]";	// The min/max combo

					InData[x].LimitIndicator = 2;

					String log = F("IFTTT: ");
					log += Ident;
					log += " is above upper limit - Trigger will be sent";
					addLog(LOG_LEVEL_INFO, log);
					IFTTT_Trigger(IFTTT_APIKey, IFTTT_Event, IFTTT_Value1,IFTTT_Value2,IFTTT_Value3);
				}

				// Check Lower Limit

				else if (LV < InData[x].LL) {

					// We are going to send Trigger - prepare stuff

					String IFTTT_APIKey = Template[0].Ident;
					String IFTTT_Event = Template[1].Ident;

					String Ident = Template[x + 2].Ident;
					Ident.trim();

					// Prepare the 3 values to be sent to IFTTT

					String IFTTT_Value1 = Ident;
					String IFTTT_Value2 = String(LV);		// This is the current value
					String IFTTT_Value3 = "[" + String(InData[x].LL) + "/" + String(InData[x].UL) + "]";	// The min/max combo

					InData[x].LimitIndicator = 1;
					String log = F("IFTTT: ");
					log += Ident;
					log += " is below lower limit - Trigger will be sent";
					addLog(LOG_LEVEL_INFO, log);
					IFTTT_Trigger(IFTTT_APIKey, IFTTT_Event, IFTTT_Value1, IFTTT_Value2, IFTTT_Value3);
				}
			}

			// Hysteresis resets

			else if (InData[x].LimitIndicator == 2) {
				if (LV < InData[x].UL - InData[x].Hysteresis) {
					InData[x].LimitIndicator = 0;
				}
			}
			else if (InData[x].LimitIndicator == 1) {
				if (LV > InData[x].LL + InData[x].Hysteresis) {
					InData[x].LimitIndicator = 0;
				}
			}
		}

        success = true;
        break;
      }

  }
  return success;
}
