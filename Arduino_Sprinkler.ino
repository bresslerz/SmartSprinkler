Skip to content
Features Business Explore Pricing
This repository
Search
Sign in or Sign up
 Watch 19  Star 38  Fork 25 d8adrvn/smart_sprinkler
 Code  Issues 3  Pull requests 0  Projects 0  Pulse  Graphs
Branch: master Find file Copy pathsmart_sprinkler/8_Zone_Controller/Arduino_8_Zone_Irrigation_Controller.ino
c27af84  on Aug 6, 2016
@d8adrvn d8adrvn Update Arduino_8_Zone_Irrigation_Controller.ino
1 contributor
RawBlameHistory     
537 lines (507 sloc)  19.3 KB

/**
* 
 ****************************************************************************************************************************
 * Irrigation Controller v3.1.1
 * Simple, elegant irrigation controller that takes advantage of the cloud and SmartThings ecosystem
 * Arduino UNO with SmartThings Shield  and an 8 Relay Module
 * Works by receiving irrigation run times from the Cloud and then builds a queue to execute
 * Will automatically shut off if power goes out and/or the queue finishes execution
 * Updates the Cloud if a station is queued and as each station turns on or off. 
 * By using the timer library, the CPU remains ready to process any change requests to the queue
 * Provides option to add additional single relay to run a master valve or pump
 * Provides software switchable option to configure relay 8 as an irrigation zone or a master valve or pump 
 *
 ****************************************************************************************************************************
 * Libraries:
 * Timer library was created by Simon Monk as modified by JChristensen  https://github.com/JChristensen/Timer
 * SmartThings library available from https://www.dropbox.com/s/8hon320qmuio8fz/Shield%20Library.zip
 * SoftwareSerial library was default library provided with Arduino IDE
 *
 ****************************************************************************************************************************
 *  Copyright 2014 Stan Dotson (stan@dotson.info) and Matthew Nichols (matt@nichols.name)
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 ****************************************************************************************************************************
 * Configuration:
 * Relays 1-8 are controlled by Arduino pins 12-6, respectively
 * 24V power source connects to common (middle) contacts on the relay
 * Irrigation valve wires use the normally open (NO) contacts on relay
 * This code works with "Active HIGH" relays, such as SeeedUino v2 relays.  
 * To use with "Acitve LOW" relays, such as SainSmart, set isActiveHigh to false
 ****************************************************************************************************************************
/// Arduino Pin Configuration
///               ______________
///               |              |
///               |         SW[] |
///               |[]RST         |
///               |         AREF |--
///               |          GND |--
///               |           13 |--Optional Pump/Master Valve (
///               |           12 |--Relay Signal Zone 1
///               |           11 |--Relay Signal Zone 2
///             --| 3.3V      10 |--Relay Signal Zone 3
/// Relay VCC   --| 5V         9 |--Relay Signal Zone 4
/// Relay Ground--| GND        8 |--Relay Signal Zone 5
///             --| GND          |
///             --| Vin        7 |--Relay Signal Zone 6
///               |            6 |--Relay Signal Zone 7
///             --| A0         5 |--Relay Signal Zone 8 /Software Configurable Pump/Master Valve
///             --| A1    ( )  4 |--
///             --| A2         3 |--X THING_RX
///             --| A3  ____   2 |--X THING_TX
///             --| A4 |    |  1 |--
///             --| A5 |    |  0 |--
///               |____|    |____|
///                    |____|
///
//*****************************************************************************
/// @note typical relay with normally open (NO) and normally closed (NC) options
///              ---------------
///              |              |
///              |              |___
///              |     ( )      |   |  'NC' normally closed (not used)
///              |              |   *
///              |              |  /
///              |     ( )      | /   'comn' common (connect 24V power from transformer to here and daisy chain to remaining 7 relays)
///              |              |
///              |              |   *
///              |     ( )      |___|  'NO' normally open (connect sprinkler valve wire here)
///              |              |
///              ---------------
///
///              note: the ground from 24V transformer is connected to common ground running to sprinkler valves
/// ****************************************************************************************************************************
/// Typical finished wiring diagram for 8 zones with option to add an additional single relay to activate a master valve or pump
///
/// Warning: Connecting a Pump Start Relay is potentially dangerous 
/// Warning: Pumps run on 120V or 220V power and incorrect wiring could result in lethal shock or fire hazard!
/// Warning: Work is best done by licensed electrician following local codes.
///
/// Common terminals are daisy chained together.
/// Sprinkler load wires are connected to NO of respective relays
/// Ground from Sprinkler power supply is tied to the Common Ground Wire from Irrigation Wire Bundle
/// Relay 8 is used as irrigation zone
/// If optional single relay is used to control pump, Arduino sketch must be modifed to: isPin4Pump = true
///
///
///         wire [ --from 14V Sprinkler Power Supply (-)
///         nut  [ --Common Ground wire from Irrigation Wire Bundle (-)
///              ---------------    
///              |           NC |   
///              |   1      CMN |===|---from 24V Sprinkler Power Supply (+)
///              |           NO |---]---to valve 1
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   2      CMN |---|
///              |           NO |---]---to valve 2
///              ---------------    |
////              ---------------   |
///              |           NC |   |
///              |   3      CMN |---|
///              |           NO |---]---to valve 3
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   4      CMN |---|
///              |           NO |---]---to valve 4
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   5      CMN |---|
///              |           NO |---]---to valve 5
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   6      CMN |---|
///              |           NO |---]---to valve 6
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   7      CMN |---|
///              |           NO |---]---to valve 7
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   8      CMN |---|
///              |           NO | --]---to valve 8   
///              ---------------    |
///                                 |
///             //////////////////  |
///                                 |
///      Optional: Single Relay for |
///                Master Valve     |
///              ---------------    |
///    pin4  ++ | VCC       NC |   |
///  Arduino     |          CMN | --| 
///     GND   -- | GND       NO | --- to Master Valve Pump Relay (low voltage only!!!!)
///              ---------------
///    **Set isPin4Pump = true to activate optional single Master Valve Pump Relay
///
/// ****************************************************************************************************************************
/// Example finished wiring diagram for 7 Zones and Software Enabled Pump or Master Valve 
///
/// Warning: Connecting a Pump Start Relay is potentiall dangerous 
/// Warning: Pumps run on 120V or 220V power and incorrect wiring could result in lethal shock or fire hazard!
/// Warning: Work is best done by licensed electrician following local codes.
///
/// After wiring the optional master valve or pump to zone 8, you must configure the device type to recognize the pump/master valve
/// In the device type, activate the pump tile which will deactivate irrigation zone 8 tile.
/// Common terminals are daisy chained together.
/// Sprinkler load wires are connected to NO of respective relays
/// Ground from Sprinkler power supply is tied to the Common Ground Wire from Irrigation Wire Bundle
/// Software switchable pump/master valve is wired to relay 8.   
///
///
///         wire {--from 14V Sprinkler Power Supply (-)
///          nut {--Common Ground wire from Irrigation Wire Bundle (-)
///     together {--Common from Pump Start Relay or Master Vavle 
///
///              ---------------    
///              |           NC |   
///              |   1      CMN |===|---from 24V Sprinkler Power Supply (+)
///              |           NO |---]---to valve 1
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   2      CMN |---|
///              |           NO |---]---to valve 2
///              ---------------    |
////              ---------------   |
///              |           NC |   |
///              |   3      CMN |---|
///              |           NO |---]---to valve 3
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   4      CMN |---|
///              |           NO |---]---to valve 4
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   5      CMN |---|
///              |           NO |---]---to valve 5
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   6      CMN |---|
///              |           NO |---]---to valve 6
///              ---------------    |
///              ---------------    |
///              |           NC |   |
///              |   7      CMN |---|
///              |           NO |---]-to valve 7
///              ---------------    |
///              ---------------    |  
///              |           NC |   | 
///              |   8      CMN |---|
///              |           NO |--- to Pump Start Relay or Master Vavle  (low voltage only!!!)
///              ---------------    
///              **In the device type, activate the pump tile which will automagically deactivate irrigation zone 8 tile.
///
/// ****************************************************************************************************************************
*/

#include <SoftwareSerial.h>   
//#include <SmartThings.h>
#include <Event.h>
#include <Timer.h>
#define PIN_THING_RX    3
#define PIN_THING_TX    2

SmartThingsCallout_t messageCallout;    // call out function forward decalaration 
SmartThings smartthing(PIN_THING_RX, PIN_THING_TX, messageCallout);  // constructor


//user configurable global variables to set before loading to Arduino
int relays = 8;  //set up before loading to Arduino (max = 8 with current code)
boolean isActiveHigh=false; //set to true if using "active high" relay, set to false if using "active low" relay
boolean isDebugEnabled=true;    // enable or disable debug in this example
boolean isPin4Pump =true;  //set to true if you add an additional relay to pin4 and use as pump or master valve.  

//set global variables
Timer t;
int trafficCop =0;  //tracks which station has the right of way (is on)
int stations=relays; //sets number of stations = number of relays. This is software configurable in device type
int relayOn = HIGH;
int relayOff = LOW;
// initialize irrigation zone variables; for readability, zone values store [1]-[8] and [0] is not used
long stationTime[] = {0,0,0,0,0,0,0,0,0};
int8_t stationTimer[] = {0,0,0,0,0,0,0,0,0}; 
int queue[]={0,0,0,0,0,0,0,0,0};  // off: 0, queued: 1, running: 2
int relay[9];

//initialize pump related variables. 
int pin4Relay = 4;  //pin4 reserved for optional additional relay to control a pump or master valve
boolean isConfigPump = false; // if true, relay8 is used as the pump/master valve switch.  Can be toggled by device tye v2.7 and later
const char* configPumpStatus = "off";
const char* pin4PumpStatus= "off";
boolean doUpdate = false;
boolean doPumpUpdate = false;
long pumpOffDelay = 0; // set number of minutes to delay turning pump or master valve off after last zone has finished running


void setup()
{
  //debug stuff
  if (isDebugEnabled) {
    // setup debug serial port
    Serial.begin(9600);         // setup serial with a baud rate of 9600
    Serial.println("setup..");  // print out 'setup..' on start
  }
  // set SmartThings Shield LED
//TODO this will need to be changed
  smartthing.shieldSetLED(0, 0, 1); // set to blue to start
  
  // setup timed actions 
  t.every(3*60L*1000L, queueManager);// double check queue to see if there is work to do
  t.every(10L*60L*1000L, timeToUpdate); //send update to smartThings hub every 10 min
 
  
  //set up relays to control irrigation valves
  if (!isActiveHigh) {
    relayOn=LOW;
    relayOff=HIGH;
  }
  
  int i=1;
  while (i<9) {
    relay[i]= 13-i;
    pinMode(relay[i], OUTPUT); 
    digitalWrite(relay[i], relayOff);
    i++;
  }

  // set up optional hardware configured pump to pin4 on Arduino
  if (isPin4Pump) {
    pinMode(pin4Relay, OUTPUT);
    digitalWrite(pin4Relay, relayOff);
  }
}

void loop() {
  
   //run timer 
  t.update();
  
  if (doUpdate) {
    sendUpdate("ok,");
    doUpdate = false;
  }
  if (doPumpUpdate) {
    sendPumpUpdate();
    doPumpUpdate = false;
  }
 
  //run smartthing logic
//TODO this will need to be changed
  smartthing.run();
  
}

//process incoming messages from SmartThings hub
//TODO this will need to be changed
void messageCallout(String message) 
{
  if (isDebugEnabled) {
    Serial.print("Received message: '");
    Serial.print(message);
    Serial.println("' "); 
  }
  char* inValue[relays+2]; //array holds any values being delivered with message (1-8) and NULL; [0] is not used
  char delimiters[] = ",";
  char charMessage[100];
  strncpy(charMessage, message.c_str(), sizeof(charMessage));
  charMessage[sizeof(charMessage) - 1] = '\0';
  inValue[0] = strtok(charMessage,delimiters);  //remove first substring as messageType
  int i=1;
  while(inValue [i-1] != NULL) {
    inValue [i] = strtok(NULL, delimiters); //remove remaining substrings as incoming values
    i++;
  }
  if (strcmp(inValue[0],"on")==0)  {   // add new station to queue
    if (strcmp(inValue[2],"0")!=0 && strcmp(inValue[2],"null")!=0) {  //do not add to queue if zero time
      int addStation=atoi(inValue[1]);
      queue[addStation]=1;
      stationTime[addStation]=atol(inValue[2])*60L*1000L;
    }
    scheduleUpdate();
  }
  if (strcmp(inValue[0],"off")==0) {
    int removeStation=atoi(inValue[1]);     
    if (removeStation==trafficCop)  {
      toggleOff();
    }
    //else remove from queue and update queue status to SmartThings
    queue[removeStation]=0;
    scheduleUpdate();
  }
  if (strcmp(inValue[0],"update")==0) {
    scheduleUpdate();
  }
  if (strcmp(inValue[0],"allOn")==0) {
    int i=1;
    while (i<stations+1) {
      if (i != trafficCop) {
        if (strcmp(inValue[i],"0")!=0 && strcmp(inValue[i],"null")!=0) {  //do not add to queue if zero time
          queue[i]=1;
        }
      }
      stationTime[i]=atoi(inValue[i])*60L*1000L;
      i++;
    }
    scheduleUpdate();
  }
  if (strcmp(inValue[0],"allOff")==0) {
    allOff();
  }
  if (strcmp(inValue[0],"advance")==0) {
    toggleOff();
  }
  if (strcmp(inValue[0],"pump")==0) {
    if (strcmp(inValue[1],"0")==0) {
      isConfigPump = false;
//      stations = relays;
      if (stations == 7) {  
        stations = 8;
      }
    }
    if (strcmp(inValue[1],"1")==0) {
      pumpOff();
    }
    if (strcmp(inValue[1],"2")==0) {
       pumpOn();
    }
    if (strcmp(inValue[1],"3")==0) {
      isConfigPump = true;
      digitalWrite(relay[8], relayOff); 
      configPumpStatus = "enabled";
//      stations = relays - 1;
      if (stations == 8) {  
        stations = 7;
      }
    }
    schedulePumpUpdate();
  }
  queueManager();
}

void scheduleUpdate() {
  doUpdate = true; 
}
void schedulePumpUpdate() {
  doPumpUpdate = true;
}
//run through queue to check to see if there is work to do
void queueManager() 
{
  int i=1;
  while (i<9) {
    if (trafficCop==0 && queue[i]==1) {
      //ready for next in line 
      trafficCop=i;
      toggleOn();
    }
    i++;
  }
}

void toggleOn() {
  queue[trafficCop]=2;
  smartthing.shieldSetLED(83, 1, 0); //Orange for relay one
  if (isConfigPump) {
    digitalWrite(relay[8], relayOn);
    configPumpStatus = "on";
  }
  if (isPin4Pump) {
    digitalWrite(pin4Relay, relayOn);
    pin4PumpStatus = "on";
  }
  digitalWrite(relay[trafficCop], relayOn);
  t.stop(stationTimer[trafficCop]); // Kill any previously started timers.
  stationTimer[trafficCop] = t.after (stationTime[trafficCop],toggleOff);
  scheduleUpdate();
  schedulePumpUpdate();
}
void toggleOff() {
  digitalWrite(relay[trafficCop], relayOff);
  smartthing.shieldSetLED(0, 0, 1);
  queue[trafficCop]=0; //remove relay from from queue
  if (maxvalue() ==0) {
    if (isConfigPump || isPin4Pump) {
      int8_t pumpOffEvent = t.after(pumpOffDelay*60L*1000L, pumpOff);  //turns off pump after delay
    }  
  }
  trafficCop=0; //ready to check queue or watch for new commmonds
  scheduleUpdate();
  schedulePumpUpdate();
}

//added allOff as requirement of SmartThings switch capability.  
void allOff() {
  int i=1;
  while(i<stations+1) {
    queue[i]=0;
    digitalWrite(relay[i], relayOff);
    i++;
  }
  if (isConfigPump || isPin4Pump) {
    int8_t pumpOffEvent = t.after(pumpOffDelay*60L*1000L, pumpOff);  //turns off pump after delay
  } 
  smartthing.shieldSetLED(0, 0, 1);
  trafficCop=0;
  scheduleUpdate();
  schedulePumpUpdate();
}

void pumpOff() {
  if (isConfigPump) { 
      digitalWrite(relay[8], relayOff); 
      configPumpStatus = "off";
  }
  if (isPin4Pump) {
      digitalWrite(pin4Relay, relayOff);
      pin4PumpStatus="off";
  }
  schedulePumpUpdate();
}

void pumpOn() {
  if (isConfigPump) { 
      digitalWrite(relay[8], relayOn); 
      configPumpStatus = "on";
  }
  if (isPin4Pump) {
      digitalWrite(pin4Relay, relayOn);
      pin4PumpStatus = "on";
  }
}

void timeToUpdate() {
  scheduleUpdate();
  schedulePumpUpdate();
}

int maxvalue () {
  int mxm = 0;
  int i=1;
  for (i=1; i<stations+1; i++) {
  if (queue[i]>mxm) {
  mxm = queue[i];
  }
}
return mxm;
};
void sendUpdate(String statusUpdate) {
  // builds a status update to send to SmartThings hub
  String action="";
  for (int i=1; i<stations+1; i++) {
    
    if (queue[i]==0) {
      action="off";
    }
    if (queue[i]==1) {
      action="q";
    }
    if (queue[i]==2) {
      action="on";
    }
    statusUpdate.concat (action + i + ",");
  }
  smartthing.send(statusUpdate);
  statusUpdate = "";
}

void sendPumpUpdate() {
  if (isConfigPump && configPumpStatus == "enabled") {
    smartthing.send("pumpAdded");
  }
  if (!isConfigPump && !isPin4Pump) {
    smartthing.send("pumpRemoved");
  }
  if (configPumpStatus == "on" || pin4PumpStatus == "on") {
    smartthing.send("onPump");
  }
  if (configPumpStatus =="off" && isConfigPump) {
    smartthing.send("offPump");
  }
  if (pin4PumpStatus =="off" && isPin4Pump) {
    smartthing.send("offPump");
  }
}
Contact GitHub API Training Shop Blog About
© 2017 GitHub, Inc. Terms Privacy Security Status Help
