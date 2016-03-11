//#######################################################################################################
//#################################### Plugin 107: Email (Demo) #########################################
//#######################################################################################################

// This is just a demo plugin. It does send an email after pressing the "Submit" button, but that's all it can do!
// Needs to be expanded in some way to be useful.

#define PLUGIN_107
#define PLUGIN_ID_107         107
#define PLUGIN_NAME_107       "Notify - Email"
#define PLUGIN_VALUENAME1_107 "email"

#define PLUGIN_107_TIMEOUT 3000

struct P107Struct
{
  char server[64];
  byte port;
  char domain[64];
  char to[64];
  char from[64];
  char subject[64];
  char body[64];
};

boolean Plugin_107(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_107;
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].Custom = true;
        Device[deviceCount].TimerOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_107);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_107));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        struct P107Struct email;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&email, sizeof(email));
        string += F("<TR><TD>Server:<TD><input type='text' size='64' maxlength='64' name='Plugin_107_server' value='");
        string += email.server;
        string += F("'>");
        string += F("<TR><TD>Server port:<TD><input type='text' size='64' maxlength='64' name='Plugin_107_port' value='");
        string += email.port;
        string += F("'>");
        string += F("<TR><TD>Domain:<TD><input type='text' size='64' maxlength='64' name='Plugin_107_domain' value='");
        string += email.domain;
        string += F("'>");
        string += F("<TR><TD>Mail To:<TD><input type='text' size='64' maxlength='64' name='Plugin_107_to' value='");
        string += email.to;
        string += F("'>");
        string += F("<TR><TD>Mail From:<TD><input type='text' size='64' maxlength='64' name='Plugin_107_from' value='");
        string += email.from;
        string += F("'>");
        string += F("<TR><TD>Subject:<TD><input type='text' size='64' maxlength='64' name='Plugin_107_subject' value='");
        string += email.subject;
        string += F("'>");
        string += F("<TR><TD>Body:<TD><input type='text' size='64' maxlength='64' name='Plugin_107_body' value='");
        string += email.body;
        string += F("'>");
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        struct P107Struct email;
        String plugin1 = WebServer.arg("Plugin_107_server");
        strncpy(email.server, plugin1.c_str(), sizeof(email.server));
        String plugin2 = WebServer.arg("Plugin_107_port");
        email.port = plugin2.toInt();
        String plugin3 = WebServer.arg("Plugin_107_domain");
        strncpy(email.domain, plugin3.c_str(), sizeof(email.domain));
        String plugin4 = WebServer.arg("Plugin_107_to");
        strncpy(email.to, plugin4.c_str(), sizeof(email.to));
        String plugin5 = WebServer.arg("Plugin_107_from");
        strncpy(email.from, plugin5.c_str(), sizeof(email.from));
        String plugin6 = WebServer.arg("Plugin_107_subject");
        strncpy(email.subject, plugin6.c_str(), sizeof(email.subject));
        String plugin7 = WebServer.arg("Plugin_107_subject");
        strncpy(email.body, plugin7.c_str(), sizeof(email.body));

        Settings.TaskDeviceID[event->TaskIndex] = 1; // temp fix, needs a dummy value

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&email, sizeof(email));
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        struct P107Struct email;
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&email, sizeof(email));
        Plugin_107_send(email.domain,email.to,email.from,email.subject,email.body,email.server,email.port);
        break;
      }

    case PLUGIN_WRITE:
      {
        String tmpString  = string;
        int argIndex = tmpString.indexOf(',');
        if (argIndex)
          tmpString = tmpString.substring(0, argIndex);
        if (tmpString.equalsIgnoreCase("email"))
        {
          event->TaskIndex=event->Par1-1;
          struct P107Struct email;
          LoadCustomTaskSettings(event->TaskIndex, (byte*)&email, sizeof(email));
          Plugin_107_send(email.domain,email.to,email.from,email.subject,email.body,email.server,email.port);
          success = true;
        }
        break;
      }
  }
  return success;
}


boolean Plugin_107_send(String aDomain , String aTo, String aFrom, String aSub, String aMesg, String aHost, int aPort)
{
  boolean myStatus = false;

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(aHost.c_str(), aPort)) {
    myStatus = false;
  }
  else {

    // Wait for Client to Start Sending
    // The MTA Exchange
    while (true) {

      if (Plugin_107_MTA(client, "",                                                   "220 ") == false) break;
      if (Plugin_107_MTA(client, "EHLO " + aDomain,                                    "250 ") == false) break;
      if (Plugin_107_MTA(client, "MAIL FROM:" + aFrom + "",                            "250 ") == false) break;
      if (Plugin_107_MTA(client, "RCPT TO:" + aTo + "",                                "250 ") == false) break;
      if (Plugin_107_MTA(client, "DATA",                                               "354 ") == false) break;
      if (Plugin_107_MTA(client, "Subject:" + aSub + "\r\n\r\n" + aMesg + "\r\n.\r\n", "250 ") == false) break;

      myStatus = true;
      break;

    }

    client.flush();
    client.stop();

    if (myStatus == true) {
      Serial.println(" Connection Closed Successfully");
    }
    else {
      Serial.println(" Connection Closed With Error");
    }

  }

  return myStatus;

}


boolean Plugin_107_MTA(WiFiClient client, String aStr, String aWaitForPattern)
{

  boolean myStatus = false;

  if (aStr.length() ) client.println(aStr);

  yield();

  // Wait For Response
  unsigned long ts = millis();
  while (true) {
    if ( ts + PLUGIN_107_TIMEOUT < millis() ) {
      myStatus = false;
      break;
    }

    yield();

    String line = client.readStringUntil('\n');

    if (line.indexOf(aWaitForPattern) >= 0) {
      myStatus = true;
      break;
    }
  }

  return myStatus;
}

