//#######################################################################################################
//#################################### Plugin 125: ArduCAM ##############################################
//#######################################################################################################

/*
 /
 / ArduCAM plugin for ESPEasy
 / the only supported device is the OV2640_MINI_2MP
 /
 / needs the library ArduCAM from:
 / https://github.com/ArduCAM/Arduino
 /
 / supported is the following:
 /  * request to take a picture an send it to the webserver client back
 /      http://<ESP IP address>/control?cmd=arducam,picture
 /  * request to take a picture and save it to the SD card
 /      http://<ESP IP address>/control?cmd=arducam,savepic
 /  * request to get a stream (this will block the ESP8266 for other things)
 /      http://<ESP IP address>/control?cmd=arducam,stream
 /  * request the last picture that was saved on the SD card
 /      http://<ESP IP address>/control?cmd=arducam,getlastpic
 /  * change the settings
 /    * change resolution for picture
 /      http://<ESP IP address>/control?cmd=arducam,picres,8
 /      0 - 160x120
 /      1 - 176x144
 /      2 - 320x240
 /      3 - 352x288
 /      4 - 640x480
 /      5 - 800x600
 /      6 - 1024x768
 /      7 - 1280x1024
 /      8 - 1600x1200
 /    * change resolution for the stream
 /      http://<ESP IP address>/control?cmd=arducam,strres,4
 /      resolutions see above
 /    * change light mode
 /      http://<ESP IP address>/control?cmd=arducam,lim,3
 /      0 - Auto
 /      1 - Sunny
 /      2 - Cloudy
 /      3 - Office
 /      4 - Home
 /    * change brightness
 /      http://<ESP IP address>/control?cmd=arducam,bri,4
 /      0 to 8  =>  +4 to -4
 /    * change saturation
 /      http://<ESP IP address>/control?cmd=arducam,sat,4
 /      0 to 8  =>  +4 to -4
 /    * change contrast
 /      http://<ESP IP address>/control?cmd=arducam,con,4
 /      0 to 8  =>  +4 to -4
 /    * enable special effects
 /      http://<ESP IP address>/control?cmd=arducam,eff,7
 /      0 to 12  => Antique, Bluish, BW, Negative, ....
 /      7 is normal mode (without effects)
 /
*/

#include <ArduCAM.h>

#define PLUGIN_125
#define PLUGIN_ID_125         125
#define PLUGIN_NAME_125       "ArduCAM OV2640 [TESTING]"
#define PLUGIN_VALUENAME1_125 "Pics"

#define CS_PIN  16
ArduCAM myCAM(OV2640, CS_PIN);

uint32_t number_of_pics = 0;
String last_picture = "";
uint8_t current_res = 9;

// File on SD Card
//  |<-----MAX_FILE_SIZE------>|
//  |<-MAX_DIR_SIZE->|
//  /ARDUCAM/20170920/153546.JPG
//  /ARDUCAM/YYYYMMDD/HHMMSS.JPG

#define MAX_DIR_SIZE   18
#define MAX_FILE_SIZE  30



boolean Plugin_125(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_125;
        Device[deviceCount].Type = DEVICE_TYPE_I2C;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = false;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_125);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_125));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        // Taskindex[0] - Resolution for picture
        uint8_t choice0 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String options0[9];
        options0[0] = F("160x120");
        options0[1] = F("176x144");
        options0[2] = F("320x240");
        options0[3] = F("352x288");
        options0[4] = F("640x480");
        options0[5] = F("800x600");
        options0[6] = F("1024x768");
        options0[7] = F("1280x1024");
        options0[8] = F("1600x1200");
        int optionValues0[9] = { 0, 1, 2, 3 ,4 ,5 ,6 ,7 ,8 };
        addFormSelector(string, F("Picture Resolution"), F("plugin_125_picres"), 9, options0, optionValues0, choice0);

        // Taskindex[1] - Resolution for the stream
        uint8_t choice1 = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
        String options1[9];
        options1[0] = F("160x120");
        options1[1] = F("176x144");
        options1[2] = F("320x240");
        options1[3] = F("352x288");
        options1[4] = F("640x480");
        options1[5] = F("800x600");
        options1[6] = F("1024x768");
        options1[7] = F("1280x1024");
        options1[8] = F("1600x1200");
        int optionValues1[9] = { 0, 1, 2, 3 ,4 ,5 ,6 ,7 ,8 };
        addFormSelector(string, F("Stream Resolution"), F("plugin_125_strres"), 9, options1, optionValues1, choice1);

        // Taskindex[2] - Light Mode
        uint8_t choice2 = Settings.TaskDevicePluginConfig[event->TaskIndex][2];
        String options2[5];
        options2[0] = F("Auto");
        options2[1] = F("Sunny");
        options2[2] = F("Cloudy");
        options2[3] = F("Office");
        options2[4] = F("Home");
        int optionValues2[5] = { 0, 1, 2, 3 ,4 };
        addFormSelector(string, F("Light mode"), F("plugin_125_lim"), 5, options2, optionValues2, choice2);

        // Taskindex[3] - Brightness
        uint8_t choice3 = Settings.TaskDevicePluginConfig[event->TaskIndex][3];
        String options3[9];
        options3[0] = F("+4");
        options3[1] = F("+3");
        options3[2] = F("+2");
        options3[3] = F("+1");
        options3[4] = F("0");
        options3[5] = F("-1");
        options3[6] = F("-2");
        options3[7] = F("-3");
        options3[8] = F("-4");
        int optionValues3[9] = { 8, 7, 6, 5 ,4 ,3, 2 ,1, 0 };
        addFormSelector(string, F("Brightness"), F("plugin_125_bri"), 9, options3, optionValues3, choice3);

        // Taskindex[4] - Saturation
        uint8_t choice4 = Settings.TaskDevicePluginConfig[event->TaskIndex][4];
        String options4[9];
        options4[0] = F("+4");
        options4[1] = F("+3");
        options4[2] = F("+2");
        options4[3] = F("+1");
        options4[4] = F("0");
        options4[5] = F("-1");
        options4[6] = F("-2");
        options4[7] = F("-3");
        options4[8] = F("-4");
        int optionValues4[9] = { 0, 1, 2, 3 ,4 ,5, 6, 7, 8 };
        addFormSelector(string, F("Saturation"), F("plugin_125_sat"), 9, options4, optionValues4, choice4);

        // Taskindex[5] - Contrast
        uint8_t choice5 = Settings.TaskDevicePluginConfig[event->TaskIndex][5];
        String options5[9];
        options5[0] = F("+4");
        options5[1] = F("+3");
        options5[2] = F("+2");
        options5[3] = F("+1");
        options5[4] = F("0");
        options5[5] = F("-1");
        options5[6] = F("-2");
        options5[7] = F("-3");
        options5[8] = F("-4");
        int optionValues5[9] = { 0, 1, 2, 3 ,4 ,5, 6, 7, 8 };
        addFormSelector(string, F("Contrast"), F("plugin_125_con"), 9, options5, optionValues5, choice5);

        // Taskindex[6] - Special Effects
        uint8_t choice6 = Settings.TaskDevicePluginConfig[event->TaskIndex][6];
        String options6[13];
        options6[0] = F("Antique");
        options6[1] = F("Bluish");
        options6[2] = F("Greenish");
        options6[3] = F("Reddish");
        options6[4] = F("BW");
        options6[5] = F("Negative");
        options6[6] = F("BWnegative");
        options6[7] = F("Normal");
        options6[8] = F("Sepia");
        options6[9] = F("Overexposure");
        options6[10] = F("Solarize");
        options6[11] = F("Blueish");
        options6[12] = F("Yellowish");
        int optionValues6[13] = { 0, 1, 2, 3 ,4 ,5, 6, 7, 8, 9, 10, 11, 12 };
        addFormSelector(string, F("Special effects"), F("plugin_125_eff"), 13, options6, optionValues6, choice6);

        // Taskindex[7] - Save picture on a SD card
        addFormCheckBox(string, F("Save to SD card"), F("plugin_125_savepic"), Settings.TaskDevicePluginConfig[event->TaskIndex][7]);
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][7])
        {
          if (!Settings.UseNTP) addFormNote(string, F("Enable and configure NTP for the filenames!"));
          if (Settings.Pin_sd_cs < 0) addFormNote(string, F("Enable and configure your SDcard!"));
        }

        addFormSubHeader(string, F("Testing"));
        addRowLabel(string, F("Test your settings"));
        addButton(string, F("/control?cmd=arducam,picture"), F("Picture"));
        addButton(string, F("/control?cmd=arducam,stream"), F("Stream"));
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][7] && Settings.UseNTP && (Settings.Pin_sd_cs >= 0))
        {
          addButton(string, F("/control?cmd=arducam,savepic"), F("Save Picture"));
        }
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_125_picres"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_125_strres"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("plugin_125_lim"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = getFormItemInt(F("plugin_125_bri"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][4] = getFormItemInt(F("plugin_125_sat"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][5] = getFormItemInt(F("plugin_125_con"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][6] = getFormItemInt(F("plugin_125_eff"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][7] = isFormItemChecked(F("plugin_125_savepic"));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        pinMode(CS_PIN, OUTPUT);
        if (plugin_arducam_check_SPI_bus() && plugin_arducam_check_ov2640())
        {
          myCAM.set_format(JPEG);
          myCAM.InitCAM();
          myCAM.OV2640_set_JPEG_size(OV2640_320x240);
          current_res = 2;
          myCAM.OV2640_set_Light_Mode(Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
          myCAM.OV2640_set_Brightness(Settings.TaskDevicePluginConfig[event->TaskIndex][3]);
          myCAM.OV2640_set_Color_Saturation(Settings.TaskDevicePluginConfig[event->TaskIndex][4]);
          myCAM.OV2640_set_Contrast(Settings.TaskDevicePluginConfig[event->TaskIndex][5]);
          myCAM.OV2640_set_Special_effects(Settings.TaskDevicePluginConfig[event->TaskIndex][6]);
          myCAM.clear_fifo_flag();
          String log = F("CAM  : ArduCAM plugin started");
          addLog(LOG_LEVEL_INFO, log);
          success = true;
          break;
        } else {
          break;
        }
      }

    case PLUGIN_WRITE:
      {
        String command = parseString(string, 1);
        if (command == F("arducam"))
        {
          String option = parseString(string, 2);
          if (option == F("getlastpic"))
          {
            success = true;
            if (last_picture == "") break;
            plugin_arducam_get_last_picture();
          }
          if (option == F("savepic"))
          {
            success = true;
            if (Settings.TaskDevicePluginConfig[event->TaskIndex][7] && Settings.UseNTP && (Settings.Pin_sd_cs >= 0))
            {
              if (current_res != Settings.TaskDevicePluginConfig[event->TaskIndex][0])
              {
                plugin_arducam_set_cam_resolution(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
              }
              uint32_t len = plugin_arducam_start_capture();
              if (!plugin_arducam_correct_picture_size(len))
              {
                break;
              }
              number_of_pics += 1;
              UserVar[event->BaseVarIndex] = number_of_pics;
              if (plugin_arducam_take_a_picture(len, true))
              {
                WebServer.send(200, "text/plain", last_picture);
              } else {
                WebServer.send(404, "text/plain", String F("Something went wrong"));
              }
            }
            else
            {
              String log = F("CAM  : Can not save picture, please look at your options!");
              addLog(LOG_LEVEL_ERROR, log);
            }
          }
          if (option == F("picture"))
          {
            success = true;
            if (current_res != Settings.TaskDevicePluginConfig[event->TaskIndex][0])
            {
              plugin_arducam_set_cam_resolution(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
            }
            uint32_t len = plugin_arducam_start_capture();
            if (!plugin_arducam_correct_picture_size(len))
            {
              break;
            }
            number_of_pics += 1;
            UserVar[event->BaseVarIndex] = number_of_pics;
            String response = F("HTTP/1.1 200 OK\r\n");
            response += F("Content-Type: image/jpeg\r\n");
            response += F("Content-len: ");
            response += String(len);
            response += F("\r\n\r\n");
            WebServer.sendContent(response);
            plugin_arducam_take_a_picture(len, false);
          }
          if (option == F("stream"))
          {
            success = true;
            if (current_res != Settings.TaskDevicePluginConfig[event->TaskIndex][1])
            {
              plugin_arducam_set_cam_resolution(Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
            }
            String log = F("CAM  : Streaming");
            addLog(LOG_LEVEL_DEBUG, log);
            String response = F("HTTP/1.1 200 OK\r\n");
            response += F("Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n");
            WebServer.sendContent(response);
            // for a stream, run this in a loop
            while (1)
            {
              uint32_t len = plugin_arducam_start_capture();
              if (plugin_arducam_correct_picture_size(len))
              {
                response = F("--frame\r\n");
                response += F("Content-Type: image/jpeg\r\n\r\n");
                WebServer.sendContent(response);
                if (!plugin_arducam_take_a_picture(len, false)) break;
              }
            }
          }
          if (option == F("picres"))
          {
            success = true;
            if (plugin_arducam_set_cam_resolution(event->Par2))
            {
              Settings.TaskDevicePluginConfig[event->TaskIndex][0] = event->Par2;
            }
          }
          if (option == F("strres"))
          {
            success = true;
            if (plugin_arducam_set_cam_resolution(event->Par2))
            {
              Settings.TaskDevicePluginConfig[event->TaskIndex][1] = event->Par2;
            }
          }
          if (option == F("lim"))
          {
            success = true;
            if (plugin_arducam_set_cam_light_mode(event->Par2))
            {
              Settings.TaskDevicePluginConfig[event->TaskIndex][2] = event->Par2;
            }
          }
          if (option == F("bri"))
          {
            success = true;
            if (plugin_arducam_set_cam_brightness(event->Par2))
            {
              Settings.TaskDevicePluginConfig[event->TaskIndex][3] = event->Par2;
            }
          }
          if (option == F("sat"))
          {
            success = true;
            if (plugin_arducam_set_cam_saturation(event->Par2))
            {
              Settings.TaskDevicePluginConfig[event->TaskIndex][4] = event->Par2;
            }
          }
          if (option == F("con"))
          {
            success = true;
            if (plugin_arducam_set_cam_saturation(event->Par2))
            {
              Settings.TaskDevicePluginConfig[event->TaskIndex][5] = event->Par2;
            }
          }
          if (option == F("eff"))
          {
            success = true;
            if (plugin_arducam_set_cam_special_effects(event->Par2))
            {
              Settings.TaskDevicePluginConfig[event->TaskIndex][6] = event->Par2;
            }
          }
        }
      }
  }
  return success;
}

boolean plugin_arducam_check_ov2640()
{
  uint8_t tries = 0;
  uint8_t vid, pid;
  boolean ret = false;
  while (tries <= 2)
  {
    String log = F("CAM  : camera module OV2640 ");
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    {
      tries += 1;
      log += F("not found! Retry: ");
      log += tries;
      delay(1000);
    } else {
      tries = 5;
      log += F("detected :)");
      ret = true;
    }
    addLog(LOG_LEVEL_INFO, log);
  }
  return ret;
}

boolean plugin_arducam_check_SPI_bus()
{
  uint8_t temp;
  uint8_t tries = 0;
  boolean ret = false;
  while(tries <= 2)
  {
    String log = F("CAM  : SPI bus ");
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55)
    {
      tries += 1;
      log += F("read error! Retry: ");
      log += tries;
      delay(1000);
    } else {
      tries = 5;
      log += F("okay.");
      ret = true;
    }
    addLog(LOG_LEVEL_INFO, log);
  }
  return ret;
}

boolean plugin_arducam_set_cam_resolution(uint8_t r)
{
  boolean ret = false;
  String log = "";
  if (r >= 0 && r <= 8)
  {
    myCAM.OV2640_set_JPEG_size(r);
    current_res = r;
    ret = true;
    log += F("CAM  : Resolution changed");
  } else {
    log += F("CAM  : Resolution not changed! Invalid option: ");
    log += r;
  }
  addLog(LOG_LEVEL_DEBUG, log);
  return ret;
}

boolean plugin_arducam_set_cam_light_mode(uint8_t m)
{
  boolean ret = false;
  String log = "";
  if (m >= 0 && m <= 4)
  {
    myCAM.OV2640_set_Light_Mode(m);
    ret = true;
    log += F("CAM  : Light mode changed");
  } else {
    log += F("CAM  : Light mode not changed! Invalid option: ");
    log += m;
  }
  addLog(LOG_LEVEL_DEBUG, log);
  return ret;
}


boolean plugin_arducam_set_cam_brightness(uint8_t b)
{
  boolean ret = false;
  String log = "";
  if (b >= 0 && b <= 8)
  {
    myCAM.OV2640_set_Brightness(b);
    ret = true;
    log += F("CAM  : Brightness changed");
  } else {
    log += F("CAM  : Brightness not changed! Invalid option: ");
    log += b;
  }
  addLog(LOG_LEVEL_DEBUG, log);
  return ret;
}


boolean plugin_arducam_set_cam_saturation(uint8_t s)
{
  boolean ret = false;
  String log = "";
  if (s >= 0 && s <= 8)
  {
    myCAM.OV2640_set_Color_Saturation(s);
    ret = true;
    log += F("CAM  : Saturation changed");
  } else {
    log += F("CAM  : Saturation not changed! Invalid option: ");
    log += s;
  }
  addLog(LOG_LEVEL_DEBUG, log);
  return ret;
}

boolean plugin_arducam_set_cam_special_effects(uint8_t e)
{
  boolean ret = false;
  String log = "";
  if (e >= 0 && e <= 8)
  {
    myCAM.OV2640_set_Special_effects(e);
    ret = true;
    log += F("CAM  : Special effect set");
  } else {
    log += F("CAM  : Special effect not set! Invalid option: ");
    log += e;
  }
  addLog(LOG_LEVEL_DEBUG, log);
  return ret;
}


uint32_t plugin_arducam_start_capture()
{
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  uint32_t len = myCAM.read_fifo_length();
  return len;
}

boolean plugin_arducam_correct_picture_size(uint32_t l)
{
  boolean ret = true;
  if (l >= 0x5FFFF)        // for OV2640_MINI_2MP = 384kb
  {
    String log = F("CAM  : Picture is over size!");
    addLog(LOG_LEVEL_ERROR, log);
    ret = false;
  }
  if (l == 0 )             //0 kb
  {
    String log = F("CAM  : Picture size is 0!");
    addLog(LOG_LEVEL_ERROR, log);
    ret = false;
  }
  return ret;
}

void plugin_arducam_get_last_picture()
{
  WiFiClient client = WebServer.client();
  File myfile;
  char sdfile[MAX_FILE_SIZE];
  byte picbuffer[256];
  int i = 0;
  uint8_t len = 0;

  last_picture.toCharArray(sdfile, last_picture.length()+1);
  if (SD.exists(sdfile)) 
  {
    myfile = SD.open(sdfile, O_READ);
    if (!myfile)
    {
      String log = F("CAM  : Open file failed!");
      addLog(LOG_LEVEL_ERROR, log);
      return;
    }
    len = myfile.size();
    if (len > 0)
    {
      String response = F("HTTP/1.1 200 OK\r\n");
      response += F("Content-Type: image/jpeg\r\n");
      response += F("Content-len: ");
      response += String(len);
      response += F("\r\n\r\n");
      WebServer.sendContent(response);

      while (myfile.available())
      {
        if (myfile.available() >= 256)
        {
          i = 256;
          myfile.read(&picbuffer[0], i);
        } else {
          i = myfile.available();
          myfile.read(&picbuffer[0], i);
        }
        if (i > 0)
        {
          client.write((const uint8_t *)&picbuffer[0], i);
          i = 0;
        }
      }
    }
    myfile.close();
  } else {
    String log = F("CAM  : Requested file does not exist!");
    addLog(LOG_LEVEL_ERROR, log);
    return;
  }
  String log = F("CAM  : get last picture");
  addLog(LOG_LEVEL_DEBUG, log);
}

boolean plugin_arducam_take_a_picture(uint32_t l, boolean save_to_sd)
{
  WiFiClient client = WebServer.client();
  File myfile;
  char sddir[MAX_DIR_SIZE];
  char sdfile[MAX_FILE_SIZE];
  uint8_t temp = 0, temp_last = 0;
  int i = 0;
  byte picbuffer[256];

  if (save_to_sd)
  {
    last_picture = "/ARDUCAM/" + getDateString();
    last_picture.toCharArray(sddir, last_picture.length()+1);
    if (!SD.exists(sddir))
    {
      if (!SD.mkdir(sddir))
      {
        String log = F("CAM  : Creating directory failed ");
        addLog(LOG_LEVEL_ERROR, log);
        return false;
      }
    }
    last_picture += "/" + getTimeString() + ".JPG";
    last_picture.toCharArray(sdfile, last_picture.length()+1);
    myfile = SD.open(sdfile, O_WRITE | O_CREAT | O_TRUNC);
    if (!myfile)
    {
      String log = F("CAM  : Open file to save picture failed!");
      addLog(LOG_LEVEL_ERROR, log);
      return false;
    }
    String log = F("CAM  : Take a picture and save it to ");
    log += last_picture;
    addLog(LOG_LEVEL_DEBUG, log);
  } else {
    String log = F("CAM  : Take a picture");
    addLog(LOG_LEVEL_DEBUG, log);
  }

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  SPI.transfer(0xFF);
  while ((temp != 0xD9) | (temp_last != 0xFF))
  {
    temp_last = temp;
    temp = SPI.transfer(0x00);

    //Write image data to buffer if not full
    if( i < 256)
    {
      picbuffer[i++] = temp;
    } else {
      //Write 256 bytes image data to file
      myCAM.CS_HIGH();
      if (save_to_sd)
      {
        myfile.write(picbuffer, 256);
      } else {
        if (!client.connected())
        {
          break;
        }
        client.write(&picbuffer[0], 256);
      }
      i = 0;
      picbuffer[i++] = temp;
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
    }
  }

  //Write the remain bytes in the buffer
  if(i > 0)
  {
    myCAM.CS_HIGH();
    if (save_to_sd)
    {
      myfile.write(picbuffer,i);
    } else {
      if (!client.connected())
      {
        return false;
      }
      client.write(&picbuffer[0], i);
    }
  }
  //Close the file
  if (save_to_sd)
  {
    myfile.close();
  }
  String log = F("CAM  : Capture done");
  addLog(LOG_LEVEL_DEBUG, log);
  return true;
}

#endif
