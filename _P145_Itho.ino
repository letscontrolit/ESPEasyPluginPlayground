//#######################################################################################################
//############################## Plugin 145: Itho ventilation unit 868Mhz remote ########################
//#######################################################################################################

// author :jodur, 10-1-2018 

// List of commands:
// 1111 to join ESP8266 with Itho ventilation unit
// 9999 to leaveESP8266 with Itho ventilation unit
// 0 to set Itho ventilation unit to standby
// 1 - set Itho ventilation unit to low speed
// 2 - set Itho ventilation unit to medium speed
// 3 - set Itho ventilation unit to high speed
// 4 - set Itho ventilation unit to full speed
// 13 - set itho to high speed with hardware timer (10 min)
// 23 - set itho to high speed with hardware timer (20 min)
// 33 - set itho to high speed with hardware timer (30 min)

//List of States:

// 1 - Itho ventilation unit to lowest speed
// 2 - Itho ventilation unit to medium speed
// 3 - Itho ventilation unit to high speed
// 4 - Itho ventilation unit to full speed
// 13 -Itho to high speed with hardware timer (10 min)
// 23 -Itho to high speed with hardware timer (20 min)
// 33 -Itho to high speed with hardware timer (30 min)

// Usage (not case sensitive):
// http://ip/control?cmd=STATE,1111
// http://ip/control?cmd=STATE,1
// http://ip/control?cmd=STATE,2
// http://ip/control?cmd=STATE,3


// This code needs the library made by 'supersjimmie': https://github.com/supersjimmie/IthoEcoFanRFT/tree/master/Master/Itho
// A CC1101 868Mhz transmitter is needed
// See https://gathering.tweakers.net/forum/list_messages/1690945 for more information

#include <SPI.h>
#include "IthoCC1101.h"
#include "IthoPacket.h"
#include <Ticker.h>

//This extra settings struct is needed because the default settingsstruct doesn't support strings
struct PLUGIN_145_ExtraSettingsStruct
{	char ID1[24];
	char ID2[24];
	char ID3[24];
} PLUGIN_145_ExtraSettings;



IthoCC1101 PLUGIN_145_rf;
void PLUGIN_145_ITHOinterrupt() ICACHE_RAM_ATTR;

// extra for interrupt handling
bool PLUGIN_145_ITHOhasPacket = false;
Ticker PLUGIN_145_ITHOticker;
int PLUGIN_145_State=1; // after startup it is assumed that the fan is running low
int PLUGIN_145_OldState=1;
int PLUGIN_145_Timer=0;
long PLUGIN_145_LastPublish=0; 
int8_t Plugin_145_IRQ_pin=-1;
bool PLUGIN_145_InitRunned = false;


#define PLUGIN_145
#define PLUGIN_ID_145         145
#define PLUGIN_NAME_145       "Itho ventilation remote"
#define PLUGIN_VALUENAME1_145 "State"
#define PLUGIN_VALUENAME2_145 "Timer"

// Timer values for hardware timer in Fan
#define PLUGIN_145_Time1      10*60
#define PLUGIN_145_Time2      20*60
#define PLUGIN_145_Time3      30*60



boolean Plugin_145(byte function, struct EventStruct *event, String& string)
{
	boolean success = false;

	switch (function)
	{

	case PLUGIN_DEVICE_ADD:
		{
			Device[++deviceCount].Number = PLUGIN_ID_145;
            Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
            Device[deviceCount].VType = SENSOR_TYPE_DUAL;
			Device[deviceCount].Ports = 0;
			Device[deviceCount].PullUpOption = false;
			Device[deviceCount].InverseLogicOption = false;
			Device[deviceCount].FormulaOption = false;
			Device[deviceCount].ValueCount = 2;
			Device[deviceCount].SendDataOption = true;
			Device[deviceCount].TimerOption = true;
			Device[deviceCount].GlobalSyncOption = true;
		   break;
		}

	case PLUGIN_GET_DEVICENAME:
		{
			string = F(PLUGIN_NAME_145);
			break;
		}

	case PLUGIN_GET_DEVICEVALUENAMES:
		{
			strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_145));
			strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_145));
			break;
		}
  
  		
	case PLUGIN_INIT:
		{
			//If configured interrupt pin differs from configured, release old pin first
			if ((Settings.TaskDevicePin1[event->TaskIndex]!=Plugin_145_IRQ_pin) && (Plugin_145_IRQ_pin!=-1))
			{
				addLog(LOG_LEVEL_DEBUG, F("IO-PIN changed, deatachinterrupt old pin"));
				detachInterrupt(Plugin_145_IRQ_pin);
			}
			LoadCustomTaskSettings(event->TaskIndex, (byte*)&PLUGIN_145_ExtraSettings, sizeof(PLUGIN_145_ExtraSettings));
			addLog(LOG_LEVEL_INFO, F("Extra Settings PLUGIN_145 loaded"));
			PLUGIN_145_rf.init();
			Plugin_145_IRQ_pin = Settings.TaskDevicePin1[event->TaskIndex];
			pinMode(Plugin_145_IRQ_pin, INPUT);
			attachInterrupt(Plugin_145_IRQ_pin, PLUGIN_145_ITHOinterrupt, RISING);
			addLog(LOG_LEVEL_INFO, F("CC1101 868Mhz transmitter initialized"));
			PLUGIN_145_rf.initReceive();
			PLUGIN_145_InitRunned=true;
			success = true;
			break;
		}

	case PLUGIN_EXIT:
	{
		addLog(LOG_LEVEL_INFO, F("EXIT PLUGIN_145"));
		//remove interupt when plugin is removed
		detachInterrupt(Plugin_145_IRQ_pin);
	}

    case PLUGIN_ONCE_A_SECOND:
    {
      //decrement timer when timermode is running
      if (PLUGIN_145_State>=10) PLUGIN_145_Timer--;
      
      //if timer has elapsed set Fan state to low
      if ((PLUGIN_145_State>=10) && (PLUGIN_145_Timer<=0)) 
       { PLUGIN_145_State=1;
         PLUGIN_145_Timer=0;
       } 
      
      //Publish new data when state is changed , timer is running or init has runned
      if  ((PLUGIN_145_OldState!=PLUGIN_145_State) || (PLUGIN_145_Timer>0) || PLUGIN_145_InitRunned)
      {
        PLUGIN_145_Publishdata(event);
        sendData(event);
		//reset flag set by init
		PLUGIN_145_InitRunned=false;
      }  
      //Remeber current state for next cycle
      PLUGIN_145_OldState=PLUGIN_145_State;
      break;
    }
    


    case PLUGIN_READ: {    
         
         // This ensures that even when Values are not changing, data is send at the configured interval for aquisition 
         PLUGIN_145_Publishdata(event);
         success = true;
         break;
    }  
    
	case PLUGIN_WRITE: {

		String tmpString = string;
		String cmd = parseString(tmpString, 1);
		String param1 = parseString(tmpString, 2);
		
			if (cmd.equalsIgnoreCase(F("STATE")))
			{

				if (param1.equalsIgnoreCase(F("1111")))
				{
					PLUGIN_145_rf.sendCommand(IthoJoin);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'join' to Itho unit"));
					printWebString += F("Sent command for 'join' to Itho unit");
					success = true;
				}

				if (param1.equalsIgnoreCase(F("9999")))
				{
					PLUGIN_145_rf.sendCommand(IthoLeave);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'leave' to Itho unit"));
					printWebString += F("Sent command for 'leave' to Itho unit");
					success = true;
				}

			   if (param1.equalsIgnoreCase(F("0")))
			    {
					PLUGIN_145_rf.sendCommand(IthoStandby);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'standby' to Itho unit"));
					printWebString += F("Sent command for 'standby' to Itho unit");
					PLUGIN_145_State=0;
					PLUGIN_145_Timer=0;
					success = true;
        }
        
				if (param1.equalsIgnoreCase(F("1")))
				{
					PLUGIN_145_rf.sendCommand(IthoLow);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'low speed' to Itho unit"));
					printWebString += F("Sent command for 'low speed' to Itho unit");
					PLUGIN_145_State=1;
					PLUGIN_145_Timer=0;
					success = true;
				}

				if (param1.equalsIgnoreCase(F("2")))
				{
					PLUGIN_145_rf.sendCommand(IthoMedium);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'medium speed' to Itho unit"));
					printWebString += F("Sent command for 'medium speed' to Itho unit");
					PLUGIN_145_State=2;
					PLUGIN_145_Timer=0;
					success = true;
				}

				if (param1.equalsIgnoreCase(F("3")))
				{
					PLUGIN_145_rf.sendCommand(IthoHigh);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'full speed' to Itho unit"));
					printWebString += F("Sent command for 'full speed' to Itho unit");
					PLUGIN_145_State=3;
					PLUGIN_145_Timer=0;
					success = true;
				}
       
				if (param1.equalsIgnoreCase(F("4")))
				{
				    PLUGIN_145_rf.sendCommand(IthoFull);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'full speed' to Itho unit"));
					printWebString += F("Sent command for 'full speed' to Itho unit");
					PLUGIN_145_State=4;
					PLUGIN_145_Timer=0;
					success = true;
				}
				if (param1.equalsIgnoreCase(F("13")))
				{
					PLUGIN_145_rf.sendCommand(IthoTimer1);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'timer 1' to Itho unit"));
					printWebString += F("Sent command for 'timer 1' to Itho unit");
					PLUGIN_145_State=13;
					PLUGIN_145_Timer=PLUGIN_145_Time1;
					success = true;
				}				

				if (param1.equalsIgnoreCase(F("23")))
				{
					PLUGIN_145_rf.sendCommand(IthoTimer2);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'timer 2' to Itho unit"));
					printWebString += F("Sent command for 'timer 2' to Itho unit");
					PLUGIN_145_State=23;
					PLUGIN_145_Timer=PLUGIN_145_Time2;
					success = true;
				}		

				if (param1.equalsIgnoreCase(F("33")))
				{
					PLUGIN_145_rf.sendCommand(IthoTimer3);
					addLog(LOG_LEVEL_INFO, F("Sent command for 'timer 3' to Itho unit"));
					printWebString += F("Sent command for 'timer 3' to Itho unit");
					PLUGIN_145_State=33;
					PLUGIN_145_Timer=PLUGIN_145_Time3;
					success = true;
				}	
			} 
      break; }
      case PLUGIN_WEBFORM_LOAD:
        {
		  addFormSubHeader(string, F("Remote RF Controls"));
          addFormTextBox(string, F("Unit ID remote 1"), F("PLUGIN_145_ID1"), PLUGIN_145_ExtraSettings.ID1, 23);
          addFormTextBox(string, F("Unit ID remote 2"), F("PLUGIN_145_ID2"), PLUGIN_145_ExtraSettings.ID2, 23);
          addFormTextBox(string, F("Unit ID remote 3"), F("PLUGIN_145_ID3"), PLUGIN_145_ExtraSettings.ID3, 23);
          success = true;
          break;
        }

      case PLUGIN_WEBFORM_SAVE:
        {
		  strcpy(PLUGIN_145_ExtraSettings.ID1, WebServer.arg(F("PLUGIN_145_ID1")).c_str());
		  strcpy(PLUGIN_145_ExtraSettings.ID2, WebServer.arg(F("PLUGIN_145_ID2")).c_str());
		  strcpy(PLUGIN_145_ExtraSettings.ID3, WebServer.arg(F("PLUGIN_145_ID3")).c_str());
		  SaveCustomTaskSettings(event->TaskIndex, (byte*)&PLUGIN_145_ExtraSettings, sizeof(PLUGIN_145_ExtraSettings));
          break;
        }	
	}

return success;
} 

void PLUGIN_145_ITHOinterrupt()
{
	PLUGIN_145_ITHOticker.once_ms(10, PLUGIN_145_ITHOcheck);
}

void PLUGIN_145_ITHOcheck()
{	Serial.print("RF signal received\n");
	if (PLUGIN_145_rf.checkForNewPacket())
	{
		IthoCommand cmd = PLUGIN_145_rf.getLastCommand();
		String Id = PLUGIN_145_rf.getLastIDstr();
		String log = F("device-ID remote: ");
		log += Id;
		log += F(" ,Command received=");
		if (PLUGIN_145_Valid_RFRemote(Id))
		{
			switch (cmd)
			{
			 case IthoUnknown:
				log += F("unknown\n");
				break;
			 case IthoStandby:
				log += F("standby\n");
				PLUGIN_145_State = 0;
				PLUGIN_145_Timer = 0;
				break;
			 case IthoLow:
				log += F("low\n");
				PLUGIN_145_State = 1;
				PLUGIN_145_Timer = 0;
				break;
			 case IthoMedium:
				log += F("medium\n");
				PLUGIN_145_State = 2;
				PLUGIN_145_Timer = 0;
				break;
			 case IthoHigh:
				log += F("high\n");
				PLUGIN_145_State = 3;
				PLUGIN_145_Timer = 0;
				break;
			 case IthoFull:
				log += F("full\n");
				PLUGIN_145_State = 4;
				PLUGIN_145_Timer = 0;
				break;
			 case IthoTimer1:
				log += +F("timer1\n");
				PLUGIN_145_State = 13;
				PLUGIN_145_Timer = PLUGIN_145_Time1;
				break;
			 case IthoTimer2:
				log += F("timer2\n");
				PLUGIN_145_State = 23;
				PLUGIN_145_Timer = PLUGIN_145_Time2;
				break;
			 case IthoTimer3:
				log += F("timer3\n");
				PLUGIN_145_State = 33;
				PLUGIN_145_Timer = PLUGIN_145_Time3;
				break;
			 case IthoJoin:
				log += F("join\n");
				break;
			 case IthoLeave:
				log += F("leave\n");
				break;
			}
			addLog(LOG_LEVEL_DEBUG, log);
		}
		else {
			log = F("Device-ID:");
			log += Id;
			log += F(" IGNORED");
				addLog(LOG_LEVEL_DEBUG, log);
			 }
	}
}
  
void PLUGIN_145_Publishdata(struct EventStruct *event){
   // Publish data when last call is at least 1 sec ago
   // This prevent high freq. changes to publish only at a rate of max 1 s
   if ((millis()-PLUGIN_145_LastPublish)>900) 
   {
    UserVar[event->BaseVarIndex]=PLUGIN_145_State;
    UserVar[event->BaseVarIndex+1]=PLUGIN_145_Timer;
    PLUGIN_145_LastPublish=millis();
    String log = F("State: ");
    log += UserVar[event->BaseVarIndex];
    addLog(LOG_LEVEL_DEBUG, log);
    log = F("Timer: ");
    log += UserVar[event->BaseVarIndex+1];
    addLog(LOG_LEVEL_DEBUG, log);
   } 
}

bool PLUGIN_145_Valid_RFRemote(String rfremoteid)
{
	return { (rfremoteid == PLUGIN_145_ExtraSettings.ID1) ||
				(rfremoteid == PLUGIN_145_ExtraSettings.ID2) ||
				(rfremoteid == PLUGIN_145_ExtraSettings.ID3)
	};
}