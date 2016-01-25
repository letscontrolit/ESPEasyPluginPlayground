//#######################################################################################################
//#################################### Plugin 101: NeoPixel clock #######################################
//#######################################################################################################
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS      114

byte Plugin_101_red = 0;
byte Plugin_101_green = 0;
byte Plugin_101_blue = 0;

Adafruit_NeoPixel *Plugin_101_pixels;

#define PLUGIN_101
#define PLUGIN_ID_101         101
#define PLUGIN_NAME_101       "NeoPixel - WordClock"
#define PLUGIN_VALUENAME1_101 "Clock"
boolean Plugin_101(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_101;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].VType = SENSOR_TYPE_SWITCH;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_101);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_101));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        char tmpString[128];
        sprintf_P(tmpString, PSTR("<TR><TD>Red:<TD><input type='text' name='plugin_101_red' size='3' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        string += tmpString;
        sprintf_P(tmpString, PSTR("<TR><TD>Green:<TD><input type='text' name='plugin_101_green' size='3' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        string += tmpString;
        sprintf_P(tmpString, PSTR("<TR><TD>Blue:<TD><input type='text' name='plugin_101_blue' size='3' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
        string += tmpString;
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg("plugin_101_red");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        String plugin2 = WebServer.arg("plugin_101_green");
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();
        String plugin3 = WebServer.arg("plugin_101_blue");
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = plugin3.toInt();
        Plugin_101_red = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        Plugin_101_green = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        Plugin_101_blue = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        Plugin_101_update();
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        if (!Plugin_101_pixels)
        {
          Plugin_101_pixels = new Adafruit_NeoPixel(NUM_LEDS, Settings.TaskDevicePin1[event->TaskIndex], NEO_GRB + NEO_KHZ800);
          Plugin_101_pixels->begin(); // This initializes the NeoPixel library.
        }
        Plugin_101_red = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        Plugin_101_green = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        Plugin_101_blue = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        success = true;
        break;
      }

    case PLUGIN_CLOCK_IN:
      {
        Plugin_101_update();
        success = true;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
        int ldrVal = map(analogRead(A0), 0, 1023, 15, 245);
        Serial.print("LDR value: ");
        Serial.println(ldrVal);
        Plugin_101_pixels->setBrightness(255-ldrVal);
        Plugin_101_pixels->show(); // This sends the updated pixel color to the hardware.
        success = true;
        break;
      }
      
    case PLUGIN_WRITE:
      {
        String tmpString  = string;
        int argIndex = tmpString.indexOf(',');
        if (argIndex)
          tmpString = tmpString.substring(0, argIndex);

        if (tmpString.equalsIgnoreCase("NeoClockColor"))
        {
          Plugin_101_red = event->Par1;
          Plugin_101_green = event->Par2;
          Plugin_101_blue = event->Par3;
          Plugin_101_update();
          success = true;
        }

        if (tmpString.equalsIgnoreCase("NeoTestAll"))
        {
          for (int i = 0; i < NUM_LEDS; i++)
            Plugin_101_pixels->setPixelColor(i, Plugin_101_pixels->Color(event->Par1, event->Par2, event->Par3));
          Plugin_101_pixels->show(); // This sends the updated pixel color to the hardware.
          success = true;
        }

        if (tmpString.equalsIgnoreCase("NeoTestLoop"))
        {
          for (int i = 0; i < NUM_LEDS; i++)
          {
            resetAndBlack();
            Plugin_101_pixels->setPixelColor(i, Plugin_101_pixels->Color(event->Par1, event->Par2, event->Par3));
            Plugin_101_pixels->show(); // This sends the updated pixel color to the hardware.
            delay(200);
          }
          success = true;
        }

        break;
      }

  }
  return success;
}

void Plugin_101_update()
{
  byte Hours = hour();
  byte Minutes = minute();

  Serial.print("Time: ");
  Serial.print(Hours);
  Serial.print(":");
  if (Minutes < 10)
    Serial.print("0");
  Serial.println(Minutes);

  resetAndBlack();
  timeToStrip(Hours, Minutes);
  Plugin_101_pixels->show(); // This sends the updated pixel color to the hardware.
}


void resetAndBlack() {
  for (int i = 0; i < NUM_LEDS; i++) {
    Plugin_101_pixels->setPixelColor(i, Plugin_101_pixels->Color(0, 0, 0));
  }
}

void pushToStrip(int ledId) {
  Plugin_101_pixels->setPixelColor(ledId, Plugin_101_pixels->Color(Plugin_101_red, Plugin_101_green, Plugin_101_blue));
}

void timeToStrip(uint8_t hours, uint8_t minutes)
{
  pushIT_IS();
  //show minutes
  if (minutes >= 5 && minutes < 10) {
    pushFIVE1();
    pushAFTER();
  } else if (minutes >= 10 && minutes < 15) {
    pushTEN1();
    pushAFTER();
  } else if (minutes >= 15 && minutes < 20) {
    pushQUATER();
    pushAFTER();
  } else if (minutes >= 20 && minutes < 25) {
    pushTEN1();
    pushFOR();
    pushHALF();
  } else if (minutes >= 25 && minutes < 30) {
    pushFIVE1();
    pushFOR();
    pushHALF();
  } else if (minutes >= 30 && minutes < 35) {
    pushHALF();
  } else if (minutes >= 35 && minutes < 40) {
    pushFIVE1();
    pushAFTER();
    pushHALF();
  } else if (minutes >= 40 && minutes < 45) {
    pushTEN1();
    pushAFTER();
    pushHALF();
  } else if (minutes >= 45 && minutes < 50) {
    pushQUATER();
    pushFOR();
  } else if (minutes >= 50 && minutes < 55) {
    pushTEN1();
    pushFOR();
  } else if (minutes >= 55 && minutes < 60) {
    pushFIVE1();
    pushFOR();
  }

  int singleMinutes = minutes % 5;
  switch (singleMinutes) {
    case 1:
      pushM_ONE();
      break;
    case 2:
      pushM_ONE();
      pushM_TWO();
      break;
    case 3:
      pushM_ONE();
      pushM_TWO();
      pushM_THREE();
      break;
    case 4:
      pushM_ONE();
      pushM_TWO();
      pushM_THREE();
      pushM_FOUR();
      break;
  }
  if (hours >= 12) {
    hours -= 12;
  }
  if (hours == 12) {
    hours = 0;
  }
  if (minutes >= 20) {
    hours++;
  }

  //show hours
  switch (hours) {
    case 0:
      pushTWELVE();
      break;
    case 1:
      pushONE();
      break;
    case 2:
      pushTWO();
      break;
    case 3:
      pushTHREE();
      break;
    case 4:
      pushFOUR();
      break;
    case 5:
      pushFIVE2();
      break;
    case 6:
      pushSIX();
      break;
    case 7:
      pushSEVEN();
      break;
    case 8:
      pushEIGHT();
      break;
    case 9:
      pushNINE();
      break;
    case 10:
      pushTEN();
      break;
    case 11:
      pushELEVEN();
      break;
    case 12:
      pushTWELVE();
      break;
  }
  //show HOUR
  if (minutes < 5) {
    pushHOURE();
  }
}

void pushM_ONE() {
  pushToStrip(0);
}
void pushM_TWO() {
  pushToStrip(12);
}
void pushM_THREE() {
  pushToStrip(101);
}
void pushM_FOUR() {
  pushToStrip(113);
}
void pushIT_IS()  {
  pushToStrip(1);
  pushToStrip(2);
  pushToStrip(3);
  pushToStrip(5);
  pushToStrip(6);
}
void pushAFTER() {
  pushToStrip(36);
  pushToStrip(37);
  pushToStrip(38);
  pushToStrip(39);
}
void pushQUATER() {
  pushToStrip(30);
  pushToStrip(31);
  pushToStrip(32);
  pushToStrip(33);
  pushToStrip(34);
}
void pushFOR() {
  pushToStrip(41);
  pushToStrip(42);
  pushToStrip(43);
  pushToStrip(44);
}
void pushHALF() {
  pushToStrip(50);
  pushToStrip(51);
  pushToStrip(52);
  pushToStrip(53);
}
void pushONE()  {
  pushToStrip(63);
  pushToStrip(64);
  pushToStrip(65);
}
void pushTWO() {
  pushToStrip(64);
  pushToStrip(65);
  pushToStrip(66);
  pushToStrip(67);
}
void pushTHREE() {
  pushToStrip(109);
  pushToStrip(110);
  pushToStrip(111);
  pushToStrip(112);
}
void pushFOUR() {
  pushToStrip(57);
  pushToStrip(58);
  pushToStrip(59);
  pushToStrip(60);
}
void pushFIVE1() {
  pushToStrip(8);
  pushToStrip(9);
  pushToStrip(10);
  pushToStrip(11);
}
void pushFIVE2() {
  pushToStrip(92);
  pushToStrip(93);
  pushToStrip(94);
  pushToStrip(95);
}
void pushSIX() {
  pushToStrip(69);
  pushToStrip(88);
  pushToStrip(91);
}
void pushSEVEN() {
  pushToStrip(69);
  pushToStrip(70);
  pushToStrip(71);
  pushToStrip(72);
  pushToStrip(73);
}
void pushEIGHT() {
  pushToStrip(97);
  pushToStrip(98);
  pushToStrip(99);
  pushToStrip(100);
}
void pushNINE() {
  pushToStrip(73);
  pushToStrip(74);
  pushToStrip(75);
  pushToStrip(76);
  pushToStrip(77);
}
void pushTEN() {
  pushToStrip(54);
  pushToStrip(59);
  pushToStrip(76);
  pushToStrip(81);
}
void pushTEN1() {
  pushToStrip(25);
  pushToStrip(26);
  pushToStrip(27);
  pushToStrip(28);
}
void pushELEVEN() {
  pushToStrip(107);
  pushToStrip(108);
  pushToStrip(109);
}
void pushTWELVE() {
  pushToStrip(82);
  pushToStrip(83);
  pushToStrip(84);
  pushToStrip(85);
  pushToStrip(86);
  pushToStrip(87);
}
void pushTWENTY() {
  pushToStrip(16);
  pushToStrip(17);
  pushToStrip(18);
  pushToStrip(19);
  pushToStrip(20);
  pushToStrip(21);
  pushToStrip(22);
}
void pushHOURE() {
  pushToStrip(102);
  pushToStrip(103);
  pushToStrip(104);
}

