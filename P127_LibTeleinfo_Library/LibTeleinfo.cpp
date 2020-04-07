// **********************************************************************************
// Driver definition for French Teleinfo
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/4.0/
//
// For any explanation about teleinfo ou use , see my blog
// http://hallard.me/category/tinfo
//
// Code based on following datasheet
// http://www.erdf.fr/sites/default/files/ERDF-NOI-CPT_02E.pdf
//
// Written by Charles-Henri Hallard (http://hallard.me)
//
// History : V1.00 2015-06-14 - First release
//
// All text above must be included in any redistribution.
//
// Edit : Tab size set to 2 but I converted tab to sapces
//
// Modifié par Dominique DAMBRAIN 2017-07-10 (http://www.dambrain.fr)
//       Version 1.0.5
//       Librairie LibTeleInfo : Allocation statique d'un tableau de stockage 
//           des variables (50 entrées) afin de proscrire les malloc/free
//           pour éviter les altérations des noms & valeurs
//       Modification en conséquence des séquences de scanning du tableau
//       ATTENTION : Nécessite probablement un ESP-8266 type Wemos D1,
//        car les variables globales occupent 42.284 octets
//
// Modifié par marc Prieur 04/2019
//	V2.0.0
//		-ajouté setReinit et getReinit.
//		-ajouté option de compilation HISTORIQUE.
//		-version standard a terminer.
//	V2.0.3
//		-taille des valeurs passées de 16 à 98 pour le mode standard TAILLE_MAX_VALUE
//		
// **********************************************************************************


#include "LibTeleinfo.h" 

int ValueItem = 0;					//Index of next position to use
struct _ValueList ValuesTab[TINFO_TABSIZE];	//Allocate static table of 50 items
									// to don't use anymore malloc & free


/* ======================================================================
Class   : TInfo
Purpose : Constructor
Input   : -
Output  : -
Comments: -
====================================================================== */
TInfo::TInfo()
{
	ValueList * me;
	for(int i = 0; i < TINFO_TABSIZE; i++) {
		me = &ValuesTab[i];
		memset(&ValuesTab[i], 0, sizeof(_ValueList) );	//Also reset the 'free' marker
		me->free=1;		//Init each entry as free
		me->flags = TINFO_FLAGS_NONE;
		if(i < TINFO_TABSIZE-1)
			me->next = &ValuesTab[i+1];
	}
	(&ValuesTab[TINFO_TABSIZE - 1])->next = NULL; //Should be OK but just in case.
  // callback
  _fn_ADPS = NULL;
  _fn_data = NULL;   
  _fn_new_frame = NULL;   
  _fn_updated_frame = NULL;   
}

/* ======================================================================
Function: init
Purpose : configure ULPNode I/O ports 
Input   : -
Output  : -
Comments: - 
====================================================================== */
void TInfo::init(boolean modeLinkyHistorique)
{
  this->modeLinkyHistorique = modeLinkyHistorique;
  // free up linked list (in case on recall init())
  listDelete();

  // clear our receive buffer
  clearBuffer();

  // We're in INIT in term of receive data
  _state = TINFO_INIT;

  need_reinit = false;
}

/* ======================================================================
Function: attachADPS 
Purpose : attach a callback when we detected a ADPS on any phase
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachADPS(void (*fn_ADPS)(uint8_t phase))
{
  // indicate the user callback
  _fn_ADPS = fn_ADPS;   
}

/* ======================================================================
Function: attachNewData 
Purpose : attach a callback when we detected a new/changed value 
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachData(void (*fn_data)(ValueList * valueslist, uint8_t  state))
{
  // indicate the user callback
  _fn_data = fn_data;   
}

/* ======================================================================
Function: attachNewFrame 
Purpose : attach a callback when we received a full frame
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachNewFrame(void (*fn_new_frame)(ValueList * valueslist))
{
  // indicate the user callback
  _fn_new_frame = fn_new_frame;   
}

/* ======================================================================
Function: attachChangedFrame 
Purpose : attach a callback when we received a full frame where data
          has changed since the last frame (cool to update data)
Input   : callback function
Output  : - 
Comments: -
====================================================================== */
void TInfo::attachUpdatedFrame(void (*fn_updated_frame)(ValueList * valueslist))
{
  // indicate the user callback
  _fn_updated_frame = fn_updated_frame;   
}

/* ======================================================================
Function: clearBuffer
Purpose : clear and init the buffer
Input   : -
Output  : -
Comments: - 
====================================================================== */
void TInfo::clearBuffer()
{
  // Clear our buffer, set index to 0
  memset(_recv_buff, 0, TINFO_BUFSIZE);
  _recv_idx = 0;
}


/* ======================================================================
Function: addCustomValue
Purpose : let user add custom values (mainly for testing)
Input   : Pointer to the label name
          pointer to the value
          pointer on flag state of the label 
Output  : pointer to the new node (or founded one)
Comments: checksum is calculated before adding, no need to bother with
====================================================================== */
ValueList * TInfo::addCustomValue(char * name, char * value, uint8_t * flags)
{
  // Little check
  if (name && *name && value && *value) {
    ValueList * me;

    // Same as if we really received this line
    customLabel(name, value, flags);
    me = valueAdd(name, value, calcChecksum(name,value), flags);
	if ( me ) {
      // something to do with new datas
      if (*flags & (TINFO_FLAGS_UPDATED | TINFO_FLAGS_ADDED | TINFO_FLAGS_ALERT) ) {
        // this frame will for sure be updated
        _frame_updated = true;
      }
      return (me);
    }
  }

  // Error or Already Exists
  return ( (ValueList *) NULL);
}

/* ======================================================================
Function: valueAdd
Purpose : Add element to the Linked List of values
Input   : Pointer to the label name
          pointer to the value
          checksum value
          flag state of the label (modified by function)
Output  : pointer to the new node (or founded one)
Comments: - state of the label changed by the function
====================================================================== */
ValueList * TInfo::valueAdd(char * name, char * value, uint8_t checksum, uint8_t * flags)
{
  
  uint8_t lgname = strlen(name);
  uint8_t lgvalue = strlen(value);
  uint8_t thischeck = calcChecksum(name,value);
	int firstfree = -1;
	ValueList * me = &ValuesTab[0];
  
	if (!validateTag(name)) { //Not a valid tag
		TI_Debug(name);
		TI_Debugln(" is not a valid tag");
		return NULL;
	}

  // just some paranoia 
	if (thischeck != checksum ) {  
		TI_Debug(name);
		TI_Debug('=');
		TI_Debug(value);
		TI_Debug(F(" '"));
		TI_Debug((char) checksum);
		TI_Debug(F("' Not added bad checksum calculated '"));
		TI_Debug((char) thischeck);
		TI_Debugln(F("'"));
	 } else  {
    // Got one and all seems good ?
    if (lgname && lgvalue && checksum) {
		// Parameters seems to be coherent
		// Scan the existing table
		int i;
		for(i=0; i < ValueItem || i < TINFO_TABSIZE; i++) {     //marc change , by ||
			me = &ValuesTab[i];
			if( ! me->free) {
				if (strncmp(me->name, name, lgname) == 0) {
					//entry found for the same value name : reuse it !
					if (strncmp(me->value, value, lgvalue) == 0) {
						*flags |= TINFO_FLAGS_EXIST;
						me->flags = *flags;
						return ( me );
		      		} else {
						//Exist, but value changed
						*flags |= TINFO_FLAGS_UPDATED;
						me->flags = *flags ;
						// Copy new value
						memset(me->value, 0, TAILLE_MAX_VALUE);
						memcpy(me->value, value , lgvalue );
						me->checksum = checksum ;

						// That's all
						return (me);	
					}
				} //name comparison
			} else {
				//This entry is free
				if(firstfree < 0)
					firstfree=i;	//It's the 1st one detected
			}
		} //for

		//No existing entry for this name : Create a new one
		if(firstfree >= 0) {
			//Use the 1st free entry found
			i=firstfree;
		} else {
			if(i < TINFO_TABSIZE)
				ValueItem=i;	//Note new entry as last one
			else
				return ( (ValueList *) NULL ); //Table saturated !
		}

      // i points the entry to use : get our buffer Safe
	  me = &ValuesTab[i];
      memset(me, 0, sizeof(_ValueList) );	//Also reset the 'free' marker
      me->checksum = checksum;
	  if(i < TINFO_TABSIZE-1)
			me->next = &ValuesTab[i+1];

	  // Copy the string data (name & value)
      memcpy(me->name, name  , lgname );
      memcpy(me->value, value , lgvalue );
 	  if ( (*flags & TINFO_FLAGS_UPDATED) == 0) {
        // so we added this node !
        *flags |= TINFO_FLAGS_ADDED ;
        me->flags = *flags;
      }
	  // That's all
	  return (me);
	 }
  } //Checksum check
  return (me);	
}	

/* ======================================================================
Function: valueRemoveFlagged
Purpose : remove element to the Linked List of values where 
Input   : paramter flags
Output  : true if found and removed
Comments: -
====================================================================== */
boolean TInfo::valueRemoveFlagged(uint8_t flags)
{
	boolean deleted = false;
	int i;
	ValueList * me;

	for (i = 0; i < ValueItem || i < TINFO_TABSIZE; i++) {    //marc change , by ||
		me = &ValuesTab[i];
		if (!me->free) {
			if (me->flags & flags) {
				//memset(me, 0, sizeof(_ValueList) );
				me->free = 1;
				deleted = true;
			}
		}
	}
	return deleted;
}

/* ======================================================================
Function: valueRemove
Purpose : remove element to the Linked List of values
Input   : Pointer to the label name
Output  : true if found and removed
Comments: -
====================================================================== */
boolean TInfo::valueRemove(char * name)
{
  boolean deleted = false;
  uint8_t lgname = strlen(name);
  int i;
	ValueList * me;

	for(i=0 ; i < ValueItem || i < 50; i++) {    //marc change , by ||
	  me = &ValuesTab[i];
	  if( ! me->free ) {
		//This entry is busy
	  	// found ?
	  	if (strncmp(me->name, name, lgname) == 0) {
			memset(me->name, 0, TAILLE_MAX_NAME);
			// free up this entry
			me->free=1;

			// and continue loop just in case we have several with same name		    
			deleted = true;
		}
	  }
	}
	return (deleted);
}

/* ======================================================================
Function: valueGet
Purpose : get value of one element
Input   : Pointer to the label name
          pointer to the value where we fill data 
Output  : pointer to the value where we filled data NULL is not found
====================================================================== */
char * TInfo::valueGet(const char * name, char * value)
{
  // Get our linked list 
  uint8_t lgname = strlen(name);
  int i;
  ValueList * me;

  // Got one and all seems good ?
  if (lgname) {
    // Loop thru the table
    for(i = 0; i < ValueItem || i < TINFO_TABSIZE; i++) {    //marc change , by ||
		me = &ValuesTab[i];
		if( ! me->free) {
      		// Check if we match this LABEL
      		if (strncmp(me->name, name, lgname) == 0) {
        		// copy to dest buffer
          		uint8_t lgvalue = strlen(me->value);
				//https://github.com/hallard/LibTeleinfo/pull/14/commits/0a8c8d87393de31f3b0f1f9c68d36790d820f64c
          		strncpy(value, me->value , lgvalue +1);
          		return ( value );
			}
        }
     } //for

  } //lgname

  // not found
  return ( NULL);
}

/* ======================================================================
Function: getTopList
Purpose : return a pointer on the top of the linked list
Input   : -
Output  : Pointer 
====================================================================== */
ValueList * TInfo::getList(void)
{
	ValueList * me = &ValuesTab[0];
  // Get our linked list 
  return me;
}

/* ======================================================================
Function: valuesDump
Purpose : dump linked list content
Input   : -
Output  : total number of values
====================================================================== */
uint8_t TInfo::valuesDump(void)
{
  // Get our linked list 
  ValueList * me = &ValuesTab[0];
  uint8_t index = 0;

  // Got one ?
  if (me) {
    // Loop thru the node
	for(int i=0; i< TINFO_TABSIZE; i++) {
      me = &ValuesTab[i];
      if( ! me->free ) {
		  index++;
		  TI_Debug(i) ;
		  TI_Debug(F(") ")) ;

		  if (me->name)
		    TI_Debug(me->name) ;
		  else
		    TI_Debug(F("NULL")) ;

		  TI_Debug(F("=")) ;

		  if (me->value)
		    TI_Debug(me->value) ;
		  else
		    TI_Debug(F("NULL")) ;

		  TI_Debug(F(" '")) ;
		  TI_Debug(me->checksum) ;
		  TI_Debug(F("' ")); 

		  // Flags management
		  if ( me->flags) {
		    TI_Debug(F("Flags:0x")); 
		    TI_Debugf("%02X =>", me->flags); 
		    if ( me->flags & TINFO_FLAGS_EXIST)
		      TI_Debug(F("Exist ")) ;
		    if ( me->flags & TINFO_FLAGS_UPDATED)
		      TI_Debug(F("Updated ")) ;
		    if ( me->flags & TINFO_FLAGS_ADDED)
		      TI_Debug(F("New ")) ;
		  }
		  TI_Debugln();
		} //test if free
    } //for
  } //me exists

  return index;
}

/* ======================================================================
Function: labelCount
Purpose : Count the number of label in the list
Input   : -
Output  : element numbers
====================================================================== */
int TInfo::labelCount()
{
  int count = 0;
	ValueList * me;
  for(int i=0 ; i < TINFO_TABSIZE; i++) {
	me = &ValuesTab[i];
	if( ! me->free)
		count++;
  }
  return (count);
}

/* ======================================================================
Function: listDelete
Purpose : Delete the ENTIRE Linked List, not a value
Input   : -
Output  : True if Ok False Otherwise
====================================================================== */
boolean TInfo::listDelete()
{

	ValueList * me;

	for(int i = 0; i < TINFO_TABSIZE; i++) {
		me = &ValuesTab[i];
		memset(&ValuesTab[i], 0, sizeof(_ValueList) );	//Also reset the 'free' marker
		me->free=1;		//Init each entry as free
		me->flags = TINFO_FLAGS_NONE;
		if(i < TINFO_TABSIZE-1)
			me->next = &ValuesTab[i+1];
	}

	return(true);
}

/* ======================================================================
Function: checksum
Purpose : calculate the checksum based on data/value fields
Input   : label name 
          label value 
Output  : checksum
Comments: return '\0' in case of error
====================================================================== */
unsigned char TInfo::calcChecksum(char *etiquette, char *valeur)
{
	uint16_t sum = ' ';
	//5.3.6. Couche liaison document enedis Enedis-NOI-CPT_54E.pdf  
	if (!this->modeLinkyHistorique)
	{
		sum = 0x09 * 2;// Somme des codes ASCII du message + un espace
	}
	// avoid dead loop, always check all is fine 
	if (etiquette && valeur) {
		// this will not hurt and may save our life ;-)
		if (strlen(etiquette) && strlen(valeur)) {
			while (*etiquette)
			sum += *etiquette++ ;
  
			while(*valeur)
			sum += *valeur++ ;
			if (this->modeLinkyHistorique)
				return ((sum & 63) + ' ');
			else
				return ((sum & 0x3f) + 0x20);
		}
	}
	return 0;
}

/* ======================================================================
Function: customLabel
Purpose : do action when received a correct label / value + checksum line
Input   : plabel : pointer to string containing the label
          pvalue : pointer to string containing the associated value
          pflags pointer in flags value if we need to cchange it
Output  : 
Comments: 
====================================================================== */
void TInfo::customLabel( char * plabel, char * pvalue, uint8_t * pflags) 
{
  int8_t phase = -1;

  // Monophasé
  if (strcmp(plabel, "ADPS")==0 ) 
    phase=0;

  // For testing
  //if (strcmp(plabel, "IINST")==0 ) {
  //  *pflags |= TINFO_FLAGS_ALERT;
  //}

  // triphasé c'est ADIR + Num Phase
  if (plabel[0]=='A' && plabel[1]=='D' && plabel[2]=='I' && plabel[3]=='R' && plabel[4]>='1' && plabel[4]<='3') {
    phase = plabel[4]-'0';
  }

  // Nous avons un ADPS ?
  if (phase>=0 && phase <=3) {
    // ne doit pas être sauvé définitivement
    *pflags |= TINFO_FLAGS_ALERT;
  
    // Traitement de l'ADPS demandé par le sketch
    if (_fn_ADPS) 
      _fn_ADPS(phase);
  }
}

/* ======================================================================
Function: checkLine
Purpose : check one line of teleinfo received
Input   : -
Output  : pointer to the data object in the linked list if OK else NULL
Comments: 
====================================================================== */
ValueList * TInfo::checkLine(char * pline) 
{
  char * p;
  char * ptok;
  char * pend;
  char * pvalue;
  char   checksum;
  char  buff[TINFO_BUFSIZE];
  uint8_t flags  = TINFO_FLAGS_NONE;
  int len ; // Group len

  if (pline==NULL)
    return NULL;

  len = strlen(pline); 

  // a line should be at least 7 Char
  // 2 Label + Space + 1 etiquette + space + checksum + \r
  if ( len < 7 )
    return NULL;

  // Get our own working copy
  strncpy( buff, _recv_buff, len+1);

  p = &buff[0];
  ptok = p;       // for sure we start with token name
  pend = p + len; // max size

  // Init values
  pvalue = NULL;
  checksum = 0;
  char separateur = ' ';
  //TI_Debug("Got [");
  //TI_Debug(len);
  //TI_Debug("] ");
  if (!this->modeLinkyHistorique)
		separateur = '\t';
  
  // Loop in buffer 
  while ( p < pend ) {
    // start of token value
	  //if (*p == ' ' && ptok) {
	if (*p == separateur && ptok) {
      // Isolate token name
      *p++ = '\0';
      // 1st space, it's the label value
      if (!pvalue)
        pvalue = p;
      else
        // 2nd space, so it's the checksum
        checksum = *p;
    }           
    // new line ? ok we got all we need ?
    if ( *p=='\r' ) {           
      *p='\0';
      // Good format ?
      if ( ptok && pvalue && checksum ) {
        // Always check to avoid bad behavior 
        if(strlen(ptok) && strlen(pvalue)) {
          // Is checksum is OK
         if ( calcChecksum(ptok,pvalue) == checksum) {
            // In case we need to do things on specific labels
            customLabel(ptok, pvalue, &flags);

            // Add value to linked lists of values
            ValueList * me = valueAdd(ptok, pvalue, checksum, &flags);

            // value correctly added/changed
            if ( me ) {
              // something to do with new datas
              if (flags & (TINFO_FLAGS_UPDATED | TINFO_FLAGS_ADDED | TINFO_FLAGS_ALERT) ) {
                // this frame will for sure be updated
                _frame_updated = true;

                // Do we need to advertise user callback
                if (_fn_data)
                  _fn_data(me, flags);
              }
            }
		 }
        }
      }
    }           
    // Next char
    p++;

  } // While

  return NULL;
}

/* ======================================================================
Function: process
Purpose : teleinfo serial char received processing, should be called
          my main loop, this will take care of managing all the other
Input   : pointer to the serial used 
Output  : teleinfo global state
====================================================================== */
void TInfo::process(char c)
{
   // be sure 7 bits only
   c &= 0x7F;

  // What we received ?
  switch (c)  {
    // start of transmission ???
    case  TINFO_STX:
      // Clear buffer, begin to store in it
      clearBuffer();

      // by default frame is not "updated"
      // if data change we'll set this flag
      _frame_updated = false;

      // We were waiting fo this one ?
      if (_state == TINFO_INIT || _state == TINFO_WAIT_STX ) {
          TI_Debugln(F("TINFO_WAIT_ETX"));
         _state = TINFO_WAIT_ETX;
      } 
    break;
    // End of transmission ?
    case  TINFO_ETX:
      // Normal working mode ?
      if (_state == TINFO_READY) {
        // Get on top of our linked list 
        ValueList * me = &_valueslist;
        
        // Call user callback if any
        if (_frame_updated && _fn_updated_frame)
          _fn_updated_frame(me);
        else if (_fn_new_frame)
		{
          _fn_new_frame(me);
 }
        #ifdef TI_Debug
          //valuesDump();
        #endif

        // It's important there since all user job is done
        // to remove the alert flags from table (ADPS for example)
        // it will be put back again next time if any
        valueRemoveFlagged(TINFO_FLAGS_ALERT);
      }

      // We were waiting fo this one ?
      if (_state == TINFO_WAIT_ETX) {
        TI_Debugln(F("TINFO_READY"));
        _state = TINFO_READY;
      } 
      else if ( _state == TINFO_INIT) {
        TI_Debugln(F("TINFO_WAIT_STX"));
        _state = TINFO_WAIT_STX ;
      } 

    break;

    // Start of group \n ?
    case  TINFO_SGR:
      // Do nothing we'll work at end of group
      // we can safely ignore this char
    break;
    // End of group \r ?
    case  TINFO_EGR:
      // Are we ready to process ?
      if (_state == TINFO_READY) {
        // Store data recceived (we'll need it)
        if ( _recv_idx < TINFO_BUFSIZE)
          _recv_buff[_recv_idx++]=c;

        // clear the end of buffer (paranoia inside)
        memset(&_recv_buff[_recv_idx], 0, TINFO_BUFSIZE-_recv_idx);

        // check the group we've just received
        checkLine(_recv_buff) ;

        // Whatever error or not, we done
        clearBuffer();
      }
    break;
    
    // other char ?
    default:
    {
      // Only in a ready state of course
      if (_state == TINFO_READY) {
        // If buffer is not full, Store data 
        if ( _recv_idx < TINFO_BUFSIZE)
          _recv_buff[_recv_idx++]=c;
        else
          clearBuffer();
      }
    }
    break;
  }
}

void TInfo::setReinit()		//marc
{
	need_reinit = true;
}

bool TInfo::getReinit() const		//marc
{
	return need_reinit;
}

//Validate that the tag is valid
bool TInfo::validateTag(String name)
{
	for (int i = 0; i < TINFO_VALIDTAG_SIZE; i++) {
		if ((validTAG[i].length() == name.length()) && (validTAG[i] == name)) {
			return true;
		}
	}
	return false; //Not an valid name !
}
