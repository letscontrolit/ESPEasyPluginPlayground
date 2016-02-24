//#######################################################################################################
//#################################### Plugin 106: Output IR ############################################
//#######################################################################################################

#include <IRremoteESP8266.h>
IRsend *Plugin_106_irSender;

#define PLUGIN_106
#define PLUGIN_ID_106         106
#define PLUGIN_NAME_106       "Infrared Transmit"

boolean Plugin_106(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_106;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
		Device[deviceCount].SendDataOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_106);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        break;
      }

    case PLUGIN_INIT:
      {
        int irPin = Settings.TaskDevicePin1[event->TaskIndex];
		if (Plugin_106_irSender == 0 && irPin != -1)
        {
			addLog(LOG_LEVEL_INFO, "INIT: IR TX");
			Plugin_106_irSender = new IRsend(irPin);
			Plugin_106_irSender->begin(); // Start the sender
        }
		if (Plugin_106_irSender != 0 && irPin == -1)
        {
		  addLog(LOG_LEVEL_INFO, "INIT: IR TX Removed");
		  delete Plugin_106_irSender;
		  Plugin_106_irSender = 0;
        }
        success = true;
        break;
      }

	case PLUGIN_TEN_PER_SECOND:
	 {
	 }

	case PLUGIN_WRITE:
	 {
		String Plugin_106_IrType;
		unsigned long Plugin_106_IrCode;
		String Plugin_106_RawCode;
		int Plugin_106_IrBits;
		char command[80];
		command[0] = 0;
		char TmpStr1[80];
		TmpStr1[0] = 0;
		string.toCharArray(command, 80);


		String tmpString = string;
		int argIndex = tmpString.indexOf(',');
		if (argIndex) tmpString = tmpString.substring(0, argIndex);

		if (GetArgv(command, TmpStr1, 2)) Plugin_106_IrType = TmpStr1;
		if (GetArgv(command, TmpStr1, 3)) Plugin_106_IrCode = strtoul(TmpStr1, NULL, 16); //(long) TmpStr1
		if (GetArgv(command, TmpStr1, 4)) Plugin_106_IrBits = str2int(TmpStr1);

		if (tmpString.equalsIgnoreCase("IRSEND") && Plugin_106_irSender != 0)
		{
			success = true;
			if (irReceiver != 0) irReceiver->disableIRIn(); // Stop the receiver

			if (Plugin_106_IrType.equalsIgnoreCase("NEC")) Plugin_106_irSender->sendNEC(Plugin_106_IrCode, Plugin_106_IrBits);
			if (Plugin_106_IrType.equalsIgnoreCase("JVC")) Plugin_106_irSender->sendJVC(Plugin_106_IrCode, Plugin_106_IrBits, 2);
			if (Plugin_106_IrType.equalsIgnoreCase("RC5")) Plugin_106_irSender->sendRC5(Plugin_106_IrCode, Plugin_106_IrBits);
			if (Plugin_106_IrType.equalsIgnoreCase("RC6")) Plugin_106_irSender->sendRC6(Plugin_106_IrCode, Plugin_106_IrBits);
			if (Plugin_106_IrType.equalsIgnoreCase("SAMSUNG")) Plugin_106_irSender->sendSAMSUNG(Plugin_106_IrCode, Plugin_106_IrBits);
			if (Plugin_106_IrType.equalsIgnoreCase("SONY")) Plugin_106_irSender->sendSony(Plugin_106_IrCode, Plugin_106_IrBits);

			addLog(LOG_LEVEL_INFO, "IR Code Sent");
			if (printToWeb)
			{
				printWebString += F("IR Code Sent ");
				printWebString += Plugin_106_IrType;
				printWebString += F("<BR>");
			}

			if (irReceiver != 0) irReceiver->enableIRIn(); // Start the receiver
		}
		break;
	}


  }
  return success;
}

