
// /*

//   This is the starting point - will add documentation here

// */

#define MQTT_MAX_PACKET_SIZE 512

// load dependancies
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Timer.h>
#include <ArduinoJson.h>
#include <avr/wdt.h>

/************ network Setup Information (CHANGE THESE FOR YOUR SETUP) ******************/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };
IPAddress ip( 192, 168, 1, 250 ); // use DHCP as preference
IPAddress dnsAddr( 192, 168, 1, 254 ); // use DHCP as preference
IPAddress gateway( 192, 168, 1, 254 ); // use DHCP as preference
IPAddress google( 64, 233, 187, 99 ); // Google - for testing network setup

/************ WIFI  Information (CHANGE THESE FOR YOUR SETUP) **************************/
bool network_wifi = false;

/*********************** MQTT setup Details ********************************************/
//const char* mqtt_server = "openHABianPine64.local";
//const char* mqtt_server = "Pauls-MacBook-Pro.local";
//const char* mqtt_server = "192.168.1.95"
IPAddress mqtt_server(192, 168, 1, 28);
const int mqtt_port = 1883;

const char mqtt_channel_pub[] = "/home/bus/action/HA-Controller/";
const char mqtt_heartbeat[] = "/home/bus/state/heartbeat/HA-Controller";
const char mqtt_channel_sub[] = "/home/bus/state/HA-Controller/#";

const char* on_cmd = "ON";
const char* off_cmd = "OFF";
const char* toggle_cmd = "TOGGLE";

/************************************ For Timer ****************************************/
Timer t;

/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(100);
const int BIG_BUFFER_SIZE = JSON_OBJECT_SIZE(256);

/**************************** define the network interface clients *********************/
EthernetClient ethClient;
PubSubClient mqttclient(ethClient);

/**************************** Borad Details ********************************************/
const char* controllerName = "HA-Controller1";
int uptime = 0;

/**************************** PIN Details **********************************************/
#define NUM_BUTTONS 8
const int button1 = 23;
const int button2 = 25;
const int button3 = 27;
const int button4 = 29;
const int button5 = 31;
const int button6 = 33;
const int button7 = 35;
const int button8 = 37;
int buttons[] = {button1,button2,button3,button4,button5,button6,button7,button8};
//int buttonStates[] = {};

const byte pushButton = 49;
const int pushButtonRelayMapping = 1;

// RELAY PIN NUMBERS
const int NUM_RELAYS = 8;
const int RELAY_ON = 0;
const int RELAY_OFF = 1;
const int relay1 = 22;
const int relay2 = 24;
const int relay3 = 26;
const int relay4 = 28;
const int relay5 = 30;
const int relay6 = 32;
const int relay7 = 34;
const int relay8 = 36;
int relays[] = {relay1,relay2,relay3,relay4,relay5,relay6,relay7,relay8};
 
/****************** State Register Details **********************************************/

const int buttonCounter1 = 0;
const int buttonCounter2 = 0;
const int buttonCounter3 = 0;
const int buttonCounter4 = 0;
const int buttonCounter5 = 0;
const int buttonCounter6 = 0;
const int buttonCounter7 = 0;
const int buttonCounter8 = 0;
int buttonCounters[] = {buttonCounter1,buttonCounter2,buttonCounter3,buttonCounter4,buttonCounter5,buttonCounter6,buttonCounter7,buttonCounter8};

// CURRENT relay state
const int buttonState1 = LOW;
const int buttonState2 = LOW;
const int buttonState3 = LOW;
const int buttonState4 = LOW;
const int buttonState5 = LOW;
const int buttonState6 = LOW;
const int buttonState7 = LOW;
const int buttonState8 = LOW;
int buttonStates[] = {buttonState1,buttonState2,buttonState3,buttonState4,buttonState5,buttonState6,buttonState7,buttonState8};
int pushbuttonState = LOW;

// LAST relay state
const int buttonLastState1 = LOW;
const int buttonLastState2 = LOW;
const int buttonLastState3 = LOW;
const int buttonLastState4 = LOW;
const int buttonLastState5 = LOW;
const int buttonLastState6 = LOW;
const int buttonLastState7 = LOW;
const int buttonLastState8 = LOW;
int buttonLastStates[] = {buttonLastState1,buttonLastState2,buttonLastState3,buttonLastState4,buttonLastState5,buttonLastState6,buttonLastState7,buttonLastState8};
int pushbuttonLastState = LOW;

// INITIAL relay state
const int relayState1 = 0;
const int relayState2 = 0;
const int relayState3 = 0;
const int relayState4 = 0;
const int relayState5 = 0;
const int relayState6 = 0;
const int relayState7 = 0;
const int relayState8 = 0;
int relayStates[] = {relayState1,relayState2,relayState3,relayState4,relayState5,relayState6,relayState7,relayState8};

/********************************** START SETUP*****************************************/
void setup()
{

  // start the serial library:
  // Set up serial port and wait until connected
  Serial.begin(9600);
  while(!Serial && !Serial.available()){}
  Serial.println("Boot Started on " + String(controllerName) + "...");

  // Pin mapping setup
  Serial.println(" Boot Start Board Config...");
  setup_pins();
  Serial.println(" Boot Start Board Config...Done");

  Serial.println(" Boot Start Network...");
  Serial.println(" Boot Start Network...Ethernet...");
  network_wifi = false;
  setup_ethernet();
  delay(1000); // wait a second
  Serial.println(" Boot Start Network...Ethernet...Done");  

  // MQTT setup
  Serial.println(" Boot Start MQTT...");
  setup_mqtt();  
  delay(1000); // wait a second
  Serial.println(" Boot Start MQTT...Done");

  // Start watchdog
  Serial.println(" Boot Start watchdog...");
  wdt_enable(WDTO_4S);
  Serial.println("   Watchdog started at 4 seconds...");
  Serial.println(" Boot Start watchdog...Done");

  // Start heartbeat pulsing
  Serial.println(" Boot Start heartbeat...");
  int heartbeart = t.every(10000, sendHeartbeat);
  Serial.print("    5 second tick started id=");
  Serial.println(heartbeart);
  Serial.println(" Boot Start heartbeat...Done");

  // Finish
  Serial.println("Boot Completed on " + String(controllerName));
  Serial.println("Waiting for action or input...");


}

/********************************** START SETUP PINS*****************************************/
void setup_pins() {

  //// Set Pins START ////
  Serial.println("Setting up I/O Pins...");
  pinMode(pushButton,INPUT_PULLUP);  
  for (int i = 0; i < NUM_RELAYS; i++) {
    String msg = "  setting button " + String(i);
    String msg2 = msg + " for pin " + String(buttons[i]);
    Serial.println(msg2);
    pinMode(buttons[i], INPUT_PULLUP); // INPUT for Switch - INPUT_PULLUP for buttons
    String msg3 = "  setting relay " + String(i);
    String msg4 = msg3 + " for pin " + String(relays[i]);
    Serial.println(msg4);
    relayStates[i] = RELAY_OFF;
    buttonStates[i] = RELAY_OFF;
    buttonLastStates[i] = RELAY_OFF;
    pinMode(relays[i], OUTPUT);
    digitalWrite(relays[i],RELAY_OFF);
  }
  
}


/****************************** START SETUP ETHERNET*****************************************/
void setup_ethernet() {

  Serial.println("Getting IP Address...");
  // if (Ethernet.begin(mac) == 0) {
  //   Serial.println("  Failed to configure Ethernet using DHCP, using default IP");
    Ethernet.begin(mac,ip);
    // Ethernet.begin(mac,ip,dnsAddr);
    // Ethernet.begin(mac,ip,dnsAddr,gateway);
  // }

  Serial.println("  Ethernet connected...");
  Serial.print("  IP address: ");
  Serial.println(Ethernet.localIP());

  delay(500); // wait half a second and test

  Serial.print("  Testing connection to internet...");

  if (ethClient.connect(google, 80)) {
    Serial.println("  connected");
    ethClient.println("  GET /search?q=arduino HTTP/1.0");
    ethClient.println();
  } else {
    Serial.println("  connection failed");
  }
  
}
    

/********************************** START SETUP MQTT*****************************************/
void setup_mqtt() {

  Serial.print("  Connecting to MQTT server...");
  Serial.println(mqtt_server);
  mqttclient.setServer(mqtt_server, mqtt_port);
  Serial.println("  #define MQTT_MAX_PACKET_SIZE :  " + String(MQTT_MAX_PACKET_SIZE));

  if (mqttclient.connect(controllerName)) {
    Serial.println("  connected... Subscribing to " + String(mqtt_channel_sub));
    sendHeartbeat();
    // mqttclient.publish(mqtt_channel_pub,"I'm alive"); 
    // ... and subscribe to topic
    mqttclient.subscribe(mqtt_channel_sub);
  }

  Serial.print("  mqtt connected status...");
  Serial.println(mqttclient.connected());
  Serial.print("  mqtt state...");
  Serial.println(mqttclient.state());
  mqttclient.setCallback(callback);

}

/********************************** FUNCTION STRING *****************************************/
void StringToChar(String input) {

  // Length (with one extra character for the null terminator)
  int str_len = input.length() + 1; 
  String output = "";
  
  // Prepare the character array (the buffer) 
  char char_array[str_len];
  
  // Copy it over 
  //output = input.toCharArray(char_array, str_len);
  //return output;
  return input.toCharArray(char_array, str_len);
  
}

/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  // if (!processJson(message)) {
  //   return false;
  // }

  //TODO sendState should be on the triggering cycle
  //sendState();
}

/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {


/*
  SAMPLE PAYLOAD:
  {
    "relay": 1,
    "action": "ON"  // ON  / OFF  / TOGGLE
  }
*/

  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(message);

  int relay = -1;

  if (!root.success()) {
    Serial.println("ERR: parseObject() json message failed");
    return false;
  }


  if (!root.containsKey("relay")) {
    Serial.println("ERR: no relay provided in message");
    return false;
  } else {
    relay = root["relay"];
  }

  if (!root.containsKey("action") ) {
    Serial.println("ERR: no action directive");
    return false;
  }

  
  if (root.containsKey("action")) {
    if (strcmp(root["action"], on_cmd) == 0) {
      relayStates[relay] = RELAY_ON;
    }
    else if (strcmp(root["action"], off_cmd) == 0) {
      relayStates[relay] = RELAY_OFF;
    }
    else if (strcmp(root["action"], toggle_cmd) == 0) {
      relayStates[relay] = !relayStates[relay];
    }
  }

  return true;
}

/********************************** START SEND Dig In ***************************************/
void sendDigitalIn(int digIn) {

  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["controller"] = controllerName;
  root["digitalIn"] = String(digIn);
  root["count"] = String(buttonCounters[digIn]);

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  mqttclient.publish(mqtt_channel_pub, buffer, true);  
}

/********************************** START SEND STATE*****************************************/
void sendState(int relay) {

  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["controller"] = controllerName;
  root["relay"] = String(relay);
  root["state"] = (relayStates[relay]) ? on_cmd : off_cmd;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  mqttclient.publish(mqtt_channel_pub, buffer, true);  
}

/***************************** START SEND HEARTBEAT *****************************************/
void sendHeartbeat() {

  Serial.println("Sending Heartbeat...");


  StaticJsonBuffer<BIG_BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["controller"] = controllerName;
  root["uptime"] = String(uptime);
  root["alive"] = on_cmd;

  JsonObject& relayStateJson = root.createNestedObject("relays");
  // Serial.println("Sending Heartbeat...3");
  for (int i = 0; i < NUM_RELAYS; i++) 
  {
  //   // Serial.println("Sending Heartbeat...3-1..." + String(i));
    // Serial.println("Sending Heartbeat...3-2..." + String((relayStates[i]) ));
    // Serial.println("Sending Heartbeat...3-2..." + String((relayStates[i]) ? on_cmd : off_cmd));
    relayStateJson[String(i)] = (relayStates[i]) ? on_cmd : off_cmd;
  }

  JsonObject& buttonCountJson = root.createNestedObject("buttonCounts");
  // Serial.println("Sending Heartbeat...4");
  for (int k = 0; k < NUM_BUTTONS; k++) 
  {
  //   // Serial.println("Sending Heartbeat...4-1..." + String(j));
  //   // Serial.println("Sending Heartbeat...4-2..." + String(buttonCounters[j]));
    // buttonCountJson["TEST"] = "123412341234123412341234";
    // buttonCountJson["TEST"+String(k)] = String(buttonCounters[k]);
    buttonCountJson[String(k)] = String(buttonCounters[k]);
    // buttonCountJson[String(j)] = String(buttonCounters[j]);
  }

  Serial.println("Sending Heartbeat...5..."+ String(root.measureLength() + 1));

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  // Serial.println("Sending Heartbeat...6");
  // Serial.println(buffer);

  if (!mqttclient.publish(mqtt_heartbeat, buffer, true)){
    Serial.println(F("Sending Heartbeat...Failed"));
  } else {
    Serial.println(F("Sending Heartbeat...Done"));
  }    

}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  Serial.println("Checking MQTT connection...");
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("  Attempting MQTT connection...");
    // Attempt to connect
   if (mqttclient.connect(controllerName)) {
      Serial.println("  connected... Subscribing to " + String(mqtt_channel_sub));
      sendHeartbeat();
      // mqttclient.publish(mqtt_channel_pub,"I'm alive"); 
      // ... and subscribe to topic
      mqttclient.subscribe(mqtt_channel_sub);
    } else {
      Serial.print("  failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      Serial.print("  status is=");
      Serial.println(mqttclient.connected());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/********************************** START MAIN LOOP*****************************************/
void loop()
{

  //safely in the loop
  wdt_reset();

  if (!mqttclient.connected()) {
    reconnect();
  }
//  mqttclient.loop();

  for (int i = 0; i < NUM_RELAYS; i++) 
  {
    buttonStates[i] = digitalRead(buttons[i]);
    if (buttonStates[i] != buttonLastStates[i]) {
      if (buttonStates[i] == LOW) {
        buttonCounters[i]++;
        Serial.println("Button " + String(i) + " pressed for " + String(buttonCounters[i]) + " times.");
        //Now toggle the relay state for the corresponding button
        relayStates[i] = !relayStates[i];
        Serial.println("Setting relay " + String(i) + " to " + String(relayStates[i]));
      } else {
        Serial.println("Button " + String(i) + " released. Relay state " + String(relayStates[i]));
      }
    }
    // save the current state as the last state,
    //for next time through the loop
    buttonLastStates[i] = buttonStates[i];
        
    //Set Relay based on stored state
    //This allows the state to be set by something else like the mqtt callback controller
    //publish a state message
    // Serial.println("Testing for state change on " + String(i) + ". Relay is " + String(digitalRead(relays[i])) + ". State is " + String(relayStates[i]) + ".");
    if (digitalRead(relays[i]) != relayStates[i]) {
      Serial.println("...Sending State...");
      sendState(i);
    }
    digitalWrite(relays[i],relayStates[i]);
    
  }

  // handle the local pushbutton
  pushbuttonState = digitalRead(pushButton);
  if (pushbuttonState != pushbuttonLastState) {
    if (pushbuttonState == LOW) {
      relayStates[pushButtonRelayMapping] = !relayStates[pushButtonRelayMapping];
        Serial.println("PushButton - Setting relay " + String(pushButtonRelayMapping) + " to " + String(relayStates[pushButtonRelayMapping]));
        digitalWrite(relays[pushButtonRelayMapping],relayStates[pushButtonRelayMapping]);
    } else {
        Serial.println("Pushbutton released.");
    }
  }
  pushbuttonLastState = pushbuttonState;

  // update the timer check
  t.update();

  // check the mqtt queue for inbound mesasges
  mqttclient.loop();

  // Delay a little bit to avoid bouncing
  delay(50);

}
