//**********************************************************************************************************************
//
//	Gets the TaskIndex from the Task Name
//
byte getTaskIndex(String TaskName)

// This routine returns the Index of the task called TaskName - note that TaskName is not case sensitive
// If TaskName is not found, then it returns 255

{
  for (byte y = 0; y < TASKS_MAX; y++)
  {
     LoadTaskSettings(y);
     String DName = ExtraTaskSettings.TaskDeviceName;
     if (( ExtraTaskSettings.TaskDeviceName[0] != 0 ) && ( DName.equalsIgnoreCase(TaskName) ))
     {
        return y;
     }  
  }
  
  return 255;
}

//  ************************************************************************************************************************
//
//	Gets the ValuenameIndex from the TaskIndex and the ValueName
//
byte getValueNameIndex(int TaskIndex, String ValueName)
{

//  This routine checks to see if ValueName is valid for a given taskindex.
//  If the Valuename is found then we return its number - else 255

   LoadTaskSettings(TaskIndex);
   for (byte y = 0; y < VARS_PER_TASK; y++)
   {
     if (ValueName.equalsIgnoreCase(ExtraTaskSettings.TaskDeviceValueNames[y]))
     {
        return y;
     }
   }
   return 255;
    
}

//  ******************************************************************************************************************
//
//	Standard routine for checking parameters
//
boolean CheckParam(String Name, int Val, int Min, int Max)

// 
{     
  if (Val < Min or Val > Max)
  {
    String log = F("ERR  : '");
    log += Name;
    log += "' has value ";
    log += Val;
    log += " but should be between ";
    log += Min;
    log += " and ";
    log += Max;
    addLog(LOG_LEVEL_ERROR, log);
      
    return false;
  }
  return true;
}
//	**************************************************************************
//
// Convert String to Integer
//
int string2Integer(String myString) { 
  int i, value, len; 
  len = myString.length(); 
  char tmp[(len+1)];       // one extra for the zero termination
  byte start=0;
  if ( myString.substring(0,1) == "-"){
    tmp[0]='-';
    start=1;   //allow a minus in front of string
  }
  for(i=start; i<len; i++) 
  { 
    tmp[i] = myString.charAt(i);
    if ( ! isdigit(tmp[i]) ) return -999;   
  } 

  tmp[i]=0; 
  value = atoi(tmp); 
  return value;  
} 

//	***************************************************************************
//
// Convert String to float
//
float string2float(String myString) { 
  int i, len; 
  float value;
  len = myString.length(); 
  char tmp[(len+1)];       // one extra for the zero termination
  byte start=0;

//  Look for decimal point - they can be anywhere but no more than one of them!

  int dotIndex=myString.indexOf(".");
  //Serial.println(dotIndex);
  
  if (dotIndex != -1)
  {
    int dotIndex2=(myString.substring(dotIndex+1)).indexOf(".");
    //Serial.println(dotIndex2);
    if (dotIndex2 != -1 )return -999.00;    // Give error if there is more than one dot
  }
  
  if ( myString.substring(0,1) == "-"){
    start=1;   //allow a minus in front of string
    tmp[i] = '-';
  }
  
  for(i=start; i<len; i++) 
  { 
    tmp[i] = myString.charAt(i);
    if ( ! isdigit(tmp[i]) ) 
    {
      if (tmp[i] != '.' )return -999;   
    }
  } 

  tmp[i]=0; 
  value = atof(tmp);
  return value;  
} 

//	*****************************************************************************
//
//	Log Updates
//
void logUpdates(byte ModNum,byte TaskNum, byte ValueNum, float NewValue)
{
        LoadTaskSettings(TaskNum);

        String log=F("");
        log+=ModNum;
        log += "  : [";
        log+=ExtraTaskSettings.TaskDeviceName;
        log+="#";
        log += ExtraTaskSettings.TaskDeviceValueNames[ValueNum];
        log += "] set to ";
        log += NewValue;
        addLog(LOG_LEVEL_INFO,log);
}

