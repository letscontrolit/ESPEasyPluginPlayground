//#######################################################################################################
//#################################### Plugin 117: RGB / LW12FC #########################################
//#######################################################################################################

//#######################################################################################################
// handle udp packets for LW12FC emulation                            
// from fhem 32_WifiLight.pm                                                                            
//#######################################################################################################

#define PLUGIN_117
#define PLUGIN_ID_117         117
#define PLUGIN_NAME_117       "RGB LW12FC"

#include <Ticker.h>

boolean Plugin_117_init = false;
WiFiUDP Plugin_117_lw12fcUDP;
int Plugin_117_FadingRate = 50; //   was 10hz
unsigned int Plugin_117_UDPCmd = 0;
unsigned int Plugin_117_SetRed = 0;
unsigned int Plugin_117_SetGreen = 0;
unsigned int Plugin_117_SetBlue = 0;

Ticker Plugin_117_Ticker;

struct Plugin_117_structPins
{
	unsigned long FadingTimer = 0;
	int CurrentLevel = 0;
	int FadingTargetLevel = 0;
	int FadingMMillisPerStep = 0;
	int FadingDirection = 0;
	int PinNo = 0;
} Plugin_117_Pins[3];

struct Plugin_117_structRGBFlasher
{
	unsigned int Count = 0;
	unsigned int OnOff = 0;
	unsigned int Freq = 0;
	unsigned int Red = 0;
	unsigned int Green = 0;
	unsigned int Blue = 0;
} Plugin_117_RGBFlasher;

struct Plugin_117_structLW12FC
{
	boolean ColourOn = false;
	boolean WhiteOn = false;
	unsigned int UDPPort = 0;
} Plugin_117_LW12FC;

boolean Plugin_117(byte function, struct EventStruct *event, String& string)
{
	boolean success = false;

	switch (function)
	{
	case PLUGIN_DEVICE_ADD:
	{
		Device[++deviceCount].Number = PLUGIN_ID_117;
		Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
		Device[deviceCount].Custom = true;
		Device[deviceCount].VType = SENSOR_TYPE_DIMMER;
		Device[deviceCount].Ports = 0;
		break;
	}

	case PLUGIN_GET_DEVICENAME:
	{
		string = F(PLUGIN_NAME_117);
		break;
	}

	case PLUGIN_GET_DEVICEVALUENAMES:
	{
		break;
	}

	case PLUGIN_WEBFORM_LOAD:
	{
		char tmpString[128];
		sprintf_P(tmpString, PSTR("<TR><TD>LW12FC UDP Port:<TD><input type='text' name='plugin_117_port' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[0]);
		string += tmpString;
		sprintf_P(tmpString, PSTR("<TR><TD>Red Pin:<TD><input type='text' name='plugin_117_RedPin' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[1]);
		string += tmpString;
		sprintf_P(tmpString, PSTR("<TR><TD>Green Pin:<TD><input type='text' name='plugin_117_GreenPin' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[2]);
		string += tmpString;
		sprintf_P(tmpString, PSTR("<TR><TD>Blue Pin:<TD><input type='text' name='plugin_117_BluePin' value='%u'>"), ExtraTaskSettings.TaskDevicePluginConfigLong[3]);
		string += tmpString;
		success = true;
		break;
	}

	case PLUGIN_WEBFORM_SAVE:
	{
		String plugin1 = WebServer.arg("plugin_117_port");
		ExtraTaskSettings.TaskDevicePluginConfigLong[0] = plugin1.toInt();
		String plugin2 = WebServer.arg("plugin_117_RedPin");
		ExtraTaskSettings.TaskDevicePluginConfigLong[1] = plugin2.toInt();
		String plugin3 = WebServer.arg("plugin_118_GreenPin");
		ExtraTaskSettings.TaskDevicePluginConfigLong[2] = plugin3.toInt();
		String plugin4 = WebServer.arg("plugin_117_BluePin");
		ExtraTaskSettings.TaskDevicePluginConfigLong[3] = plugin4.toInt();
		SaveTaskSettings(event->TaskIndex);
		success = true;
		break;
	}

	case PLUGIN_INIT:
	{
		LoadTaskSettings(event->TaskIndex);

		//udp port
		Plugin_117_LW12FC.UDPPort = ExtraTaskSettings.TaskDevicePluginConfigLong[0];
		if (Plugin_117_LW12FC.UDPPort != 0)
		{
			if (Plugin_117_lw12fcUDP.begin(Plugin_117_LW12FC.UDPPort)) addLog(LOG_LEVEL_INFO, "INIT: LW12FC UDP");
		}

		//rgb gpio pins
		boolean SetupTimer = false;
		for (int PinIndex = 0; PinIndex < 3; PinIndex++)
		{
			Plugin_117_Pins[PinIndex].PinNo = ExtraTaskSettings.TaskDevicePluginConfigLong[PinIndex + 1];
			if (Plugin_117_Pins[PinIndex].PinNo != 0)
			{
				pinMode(Plugin_117_Pins[PinIndex].PinNo, OUTPUT);
				digitalWrite(Plugin_117_Pins[PinIndex].PinNo, LOW);
				SetupTimer = true;
			}
		}

		if (SetupTimer == true)
		{
			addLog(LOG_LEVEL_INFO, "INIT: LW12FC Fading Timer");
			Plugin_117_Ticker.attach_ms(20, Plugin_117_FadingTimer);
		}

		Plugin_117_init = true;
		success = true;
		break;
	}

	case PLUGIN_TEN_PER_SECOND:
	{
		if (Plugin_117_init)
		{

			// UDP events for LW12FC emulation
			if (Plugin_117_LW12FC.UDPPort != 0)
			{
				int packetSize = Plugin_117_lw12fcUDP.parsePacket();
				if (packetSize)
				{
				//RGB my $msg = sprintf("%c%c%c%c%c%c%c%c%c", 0x7E, 0x07, 0x05, 0x03, $rr,   $rg,  $rb, 0x00, 0xEF);
				//on   my $on = sprintf("%c%c%c%c%c%c%c%c%c", 0x7E, 0x04, 0x04, 0x01, 0xFF, 0xFF, 0xFF, 0x00, 0xEF);
					char packetBuffer[128];
					int len = Plugin_117_lw12fcUDP.read(packetBuffer, 128);
					if ((len == 9) && (packetBuffer[0] == 0x7E) && 
						((packetBuffer[1] == 0x07) || (packetBuffer[1] == 0x04)))
					{
					//Serial.println("Commands received ");
					//Serial.println(int(packetBuffer[0]));
					//Serial.println(int(packetBuffer[1]));
					//Serial.println(int(packetBuffer[2]));
						Plugin_117_UDPCmd = packetBuffer[1];
						Plugin_117_SetRed = packetBuffer[4];
						Plugin_117_SetGreen = packetBuffer[5];
						Plugin_117_SetBlue = packetBuffer[6];
						Plugin_117_ProcessUDP();
					}
				}
			}

			//RGB flashing
			if (Plugin_117_RGBFlasher.Count > 0 && millis() > Plugin_117_RGBFlasher.Freq)
			{
				Plugin_117_RGBFlasher.Freq = millis() + 500; //half second flash rate
				Plugin_117_RGBFlasher.OnOff = 1 - Plugin_117_RGBFlasher.OnOff;
				if (Plugin_117_RGBFlasher.OnOff == 1)
				{
					analogWrite(Plugin_117_Pins[0].PinNo, Plugin_117_RGBFlasher.Red);
					analogWrite(Plugin_117_Pins[1].PinNo, Plugin_117_RGBFlasher.Green);
					analogWrite(Plugin_117_Pins[2].PinNo, Plugin_117_RGBFlasher.Blue);
				}
				else
				{
					analogWrite(Plugin_117_Pins[0].PinNo, 0);
					analogWrite(Plugin_117_Pins[1].PinNo, 0);
					analogWrite(Plugin_117_Pins[2].PinNo, 0);
					Plugin_117_RGBFlasher.Count = Plugin_117_RGBFlasher.Count - 1;
				}
				if (Plugin_117_RGBFlasher.Count == 0)
				{
					if (Plugin_117_LW12FC.ColourOn == true)
					{
						addLog(LOG_LEVEL_INFO, "Restoring to colour...");
						analogWrite(Plugin_117_Pins[0].PinNo, Plugin_117_Pins[0].CurrentLevel);
						analogWrite(Plugin_117_Pins[1].PinNo, Plugin_117_Pins[1].CurrentLevel);
						analogWrite(Plugin_117_Pins[2].PinNo, Plugin_117_Pins[2].CurrentLevel);
					}
					addLog(LOG_LEVEL_INFO, "Flashing RGB complete");
				}
			}

			//Fading - moved to timer section
			

		}
		
		success = true;
		break;
	}

	case PLUGIN_WRITE:
	{
		int Par[8];
		char command[80];
		command[0] = 0;
		char TmpStr1[80];
		TmpStr1[0] = 0;

		string.toCharArray(command, 80);
		Par[1] = 0;
		Par[2] = 0;
		Par[3] = 0;
		Par[4] = 0;
		Par[5] = 0;
		Par[6] = 0;
		Par[7] = 0;

		String tmpString = string;
		int argIndex = tmpString.indexOf(',');
		if (argIndex) tmpString = tmpString.substring(0, argIndex);

		if (GetArgv(command, TmpStr1, 2)) Par[1] = str2int(TmpStr1);
		if (GetArgv(command, TmpStr1, 3)) Par[2] = str2int(TmpStr1);
		if (GetArgv(command, TmpStr1, 4)) Par[3] = str2int(TmpStr1);
		if (GetArgv(command, TmpStr1, 5)) Par[4] = str2int(TmpStr1);
		if (GetArgv(command, TmpStr1, 6)) Par[5] = str2int(TmpStr1);
		if (GetArgv(command, TmpStr1, 7)) Par[6] = str2int(TmpStr1);
		if (GetArgv(command, TmpStr1, 8)) Par[7] = str2int(TmpStr1);

		//initialise LED Flashing if not flashing already
		if (tmpString.equalsIgnoreCase("RGBFLASH") && Plugin_117_RGBFlasher.Count == 0 && Par[1] <= 1023 && Par[2] <= 1023 && Par[3] <= 1023 && Par[4] > 0 && Par[4] <= 20)
		{
			success = true;
			Plugin_117_RGBFlasher.Red = Par[1];
			Plugin_117_RGBFlasher.Green = Par[2];
			Plugin_117_RGBFlasher.Blue = Par[3];
			Plugin_117_RGBFlasher.Count = Par[4];
			Plugin_117_RGBFlasher.OnOff = 0;
			Plugin_117_RGBFlasher.Freq = millis() + 500;

			//conclude any ongoing rgb fades
			for (int PinIndex = 0; PinIndex < 3; PinIndex++)
			{
				if (Plugin_117_Pins[PinIndex].FadingDirection != 0)
				{
					Plugin_117_Pins[PinIndex].FadingDirection = 0;
					Plugin_117_Pins[PinIndex].CurrentLevel = Plugin_117_Pins[PinIndex].FadingTargetLevel;
				}	
			}

			if (printToWeb)
			{
				printWebString += F("RGB flashing for ");
				printWebString += Par[4];
				printWebString += F(" times.");
				printWebString += F("<BR>");
			}
			addLog(LOG_LEVEL_INFO, "Start PWM Flash");
		}

		//initialise LED Fading pin 0=r,1=g,2=b
		if (tmpString.equalsIgnoreCase("PWMFADE"))
		{
			success = true;
			if (Par[2] >= 0 && Par[2] <= 1023 && Par[1] >= 0 && Par[1] <= 3 && Par[3] > 0 && Par[3] < 30)
			{	
				if (Par[1] == 3 || (Plugin_117_RGBFlasher.Count == 0 && Plugin_117_RGBFlasher.OnOff == 0)) //white pin or no flashing going so init fade
				{
					Plugin_117_Pins[Par[1]].FadingTargetLevel = Par[2];
					Plugin_117_Pins[Par[1]].FadingMMillisPerStep = 1000 / Plugin_117_FadingRate;
					Plugin_117_Pins[Par[1]].FadingDirection = (abs(Plugin_117_Pins[Par[1]].FadingTargetLevel - Plugin_117_Pins[Par[1]].CurrentLevel)) / (Plugin_117_FadingRate * Par[3]);
					if (Plugin_117_Pins[Par[1]].FadingDirection == 0) { Plugin_117_Pins[Par[1]].FadingDirection = 1; }
					if (Plugin_117_Pins[Par[1]].CurrentLevel == Plugin_117_Pins[Par[1]].FadingTargetLevel) { Plugin_117_Pins[Par[1]].FadingDirection = 0; }
					if (Plugin_117_Pins[Par[1]].CurrentLevel > Plugin_117_Pins[Par[1]].FadingTargetLevel) { Plugin_117_Pins[Par[1]].FadingDirection = Plugin_117_Pins[Par[1]].FadingDirection * -1; }
					Plugin_117_Pins[Par[1]].FadingTimer = millis();
					if (printToWeb)
					{
						printWebString += F("PWM fading over ");
						printWebString += Par[3];
						printWebString += F(" seconds.");
						printWebString += F("<BR>");
					}
					addLog(LOG_LEVEL_INFO, "Start PWM Fade");
				}
				else // currently flashing so set fade completed
				{
					Plugin_117_Pins[Par[1]].FadingTargetLevel = Par[2];
					Plugin_117_Pins[Par[1]].FadingDirection = 0;
					Plugin_117_Pins[Par[1]].CurrentLevel = Plugin_117_Pins[Par[1]].FadingTargetLevel;
				}
			}
		}

		break;
 		}
	}

        return success;
}

/************************/
/* handle fading timer */
/***********************/
void Plugin_117_FadingTimer()
{
	//Fading
	for (int PinIndex = 0; PinIndex < 3; PinIndex++)
	{
		if (Plugin_117_Pins[PinIndex].FadingDirection != 0)
		{
			if (millis() > Plugin_117_Pins[PinIndex].FadingTimer)
			{
				Plugin_117_Pins[PinIndex].FadingTimer = millis() + Plugin_117_Pins[PinIndex].FadingMMillisPerStep;
				Plugin_117_Pins[PinIndex].CurrentLevel = Plugin_117_Pins[PinIndex].CurrentLevel + Plugin_117_Pins[PinIndex].FadingDirection;
				if (Plugin_117_Pins[PinIndex].CurrentLevel >= Plugin_117_Pins[PinIndex].FadingTargetLevel && Plugin_117_Pins[PinIndex].FadingDirection > 0)
				{
					Plugin_117_Pins[PinIndex].FadingDirection = 0;
					Plugin_117_Pins[PinIndex].CurrentLevel = Plugin_117_Pins[PinIndex].FadingTargetLevel;
					addLog(LOG_LEVEL_INFO, "Fade up complete");
				}
				if (Plugin_117_Pins[PinIndex].CurrentLevel <= Plugin_117_Pins[PinIndex].FadingTargetLevel && Plugin_117_Pins[PinIndex].FadingDirection < 0)
				{
					Plugin_117_Pins[PinIndex].FadingDirection = 0;
					Plugin_117_Pins[PinIndex].CurrentLevel = Plugin_117_Pins[PinIndex].FadingTargetLevel;
					addLog(LOG_LEVEL_INFO, "Fade down complete");
				}
				analogWrite(Plugin_117_Pins[PinIndex].PinNo, Plugin_117_Pins[PinIndex].CurrentLevel);
			}
		}
	}
}

/**********************************************************************/
/* handle udp packets for LW12FC emulation                            */
/* from fhem 32_WifiLight.pm : 										  */
/* RGB my $msg = sprintf("%c%c%c%c%c%c%c%c%c", 0x7E, 0x07, 0x05, 0x03, $rr,   $rg,  $rb, 0x00, 0xEF);*/
/* on  my $on  = sprintf("%c%c%c%c%c%c%c%c%c", 0x7E, 0x04, 0x04, 0x01, 0xFF, 0xFF, 0xFF, 0x00, 0xEF);*/
/**********************************************************************/
void Plugin_117_ProcessUDP()
{
	boolean LW12FCUpdate = false;
	switch (int(Plugin_117_UDPCmd))
	{
	case 0x07:	 //set colour / off
		LW12FCUpdate = true;
		if ((Plugin_117_SetRed == 0) && (Plugin_117_SetGreen == 0) && (Plugin_117_SetBlue == 0)) 
			Plugin_117_LW12FC.ColourOn = false;
		else 
			Plugin_117_LW12FC.ColourOn = true;
		break;
	case 0x04: 	 //on
		Plugin_117_LW12FC.ColourOn = true;
		LW12FCUpdate = true;
		break;
	}

	if (LW12FCUpdate == true)
	{
		if (Plugin_117_LW12FC.ColourOn == true)
		{
			// 0-255 -> 0-1023
			Plugin_117_Pins[0].CurrentLevel = (Plugin_117_SetRed * 4) + (Plugin_117_SetRed / 85); 
			Plugin_117_Pins[1].CurrentLevel = (Plugin_117_SetGreen * 4) + (Plugin_117_SetGreen / 85);
			Plugin_117_Pins[2].CurrentLevel = (Plugin_117_SetBlue * 4) + (Plugin_117_SetBlue / 85);
				if (Plugin_117_RGBFlasher.Count == 0) //only change led colour if not flashing, selected colour will be applied after flashing concludes
			{
				analogWrite(Plugin_117_Pins[0].PinNo, Plugin_117_Pins[0].CurrentLevel);
				analogWrite(Plugin_117_Pins[1].PinNo, Plugin_117_Pins[1].CurrentLevel);
				analogWrite(Plugin_117_Pins[2].PinNo, Plugin_117_Pins[2].CurrentLevel);
				//Serial.println("Setting RGB To:");
				//Serial.println(Plugin_117_Pins[0].CurrentLevel);
				//Serial.println(Plugin_117_Pins[1].CurrentLevel);
				//Serial.println(Plugin_117_Pins[2].CurrentLevel);
			}
		}
		else
		{
			if (Plugin_117_RGBFlasher.Count == 0) //only change led colour if not flashing, selected colour will be applied after flashing concludes
			{
				analogWrite(Plugin_117_Pins[0].PinNo, 0);
				analogWrite(Plugin_117_Pins[1].PinNo, 0);
				analogWrite(Plugin_117_Pins[2].PinNo, 0);
			}
		}
	}
}
