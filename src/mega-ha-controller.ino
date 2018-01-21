
// /*

//   This is the starting point - will add documentation here

// */

// size of buffer used for MQTT messages
#define MQTT_MAX_PACKET_SIZE 512
// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

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
// IPAddress dnsAddr( 192, 168, 1, 254 ); // use DHCP as preference
// IPAddress gateway( 192, 168, 1, 254 ); // use DHCP as preference
IPAddress google( 64, 233, 187, 99 ); // Google - for testing network setup

/************ MQTT  Information (CHANGE THESE FOR YOUR SETUP) **************************/
bool mqtt_active = true;

/***************************** web server seup *****************************************/
//EthernetServer webserver(80);        // create a server at port 80
// File webFile;                     // the web page file on the SD card
//char HTTP_req[REQ_BUF_SZ] = {0};  // buffered HTTP request stored as null terminated string
//char req_index = 0;               // index into HTTP_req buffer

/**************************** Board Details ********************************************/
const char* controllerName = "HA-Controller1";
int uptime = 0;

/*********************** MQTT setup Details ********************************************/
IPAddress mqtt_server(192, 168, 1, 28);
const int mqtt_port = 1883;

//const char controllerID = "1"
const char mqtt_channel_pub[] = "/home/bus/action/HA-Controller/1/";
const char mqtt_channel_sub[] = "/home/bus/state/HA-Controller/1/#";
// const char* mqtt_channel_sub2[] = {"/home/bus/state/HA-Controller/" + controllerName + "/#"};

char mqtt_heartbeat[] = "/home/bus/state/heartbeat/HA-Controller";

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

// const byte redled = 45;
// const byte greenled = 47;

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
const int relayState1 = 1;
const int relayState2 = 1;
const int relayState3 = 1;
const int relayState4 = 1;
const int relayState5 = 1;
const int relayState6 = 1;
const int relayState7 = 1;
const int relayState8 = 1;
int relayStates[] = {relayState1,relayState2,relayState3,relayState4,relayState5,relayState6,relayState7,relayState8};

/********************************** START SETUP*****************************************/
void setup()
{

  // start the serial library:
  // Set up serial port and wait until connected
  Serial.begin(9600);
  while(!Serial && !Serial.available()){}
  Serial.println("Boot Started on " + String(controllerName) + "...");

  Serial.println(" Boot Start Network...");
  Serial.println(" Boot Start Network...Ethernet...");
  setup_ethernet();
  delay(1000); // wait a second
  Serial.println(" Boot Start Network...Ethernet...Done");  

  // MQTT setup
  Serial.println(" Boot Start MQTT...");
  setup_mqtt();  
  delay(1000); // wait a second
  Serial.println(" Boot Start MQTT...Done");

  // Pin mapping setup
  Serial.println(" Boot Start Board Config...");
  setup_pins();
  Serial.println(" Boot Start Board Config...Done");

  // Start heartbeat pulsing
  Serial.println(" Boot Start heartbeat...");
  int heartbeart = t.every(10000, sendHeartbeat);
  int second_count = t.every(1000, updateUptime);
  Serial.print("    10 second tick started id=");
  Serial.println(heartbeart);
  Serial.print("    uptime counter started id=");
  Serial.println(second_count);
  Serial.println(" Boot Start heartbeat...Done");

  // Start watchdog
  Serial.println(" Boot Start watchdog...");
  wdt_enable(WDTO_4S);
  Serial.println("   Watchdog started at 4 seconds...");
  Serial.println(" Boot Start watchdog...Done");

  // Finish
  Serial.println("Boot Completed on " + String(controllerName));
  Serial.println("Waiting for action or input...");


}

/********************************** INCREMENT UPTIME COUNTER ********************************/
void updateUptime(){
  uptime++;
}

/********************************** START SETUP PINS*****************************************/
void setup_pins() {

  //// Set Pins START ////
  Serial.println("Setting up I/O Pins...");
  pinMode(pushButton,INPUT_PULLUP); 
  pushbuttonState = LOW; 
  // pinMode(redled,OUTPUT);
  // pinMode(greenled,OUTPUT);
  // set redled On
  // digitalWrite(redled,HIGH);

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
    digitalWrite(relays[i],RELAY_OFF); //NC relay
    //sendHeartbeat();
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

  // Serial.println("  Creating the web server on the host...");
  // webserver.begin();
  // Serial.println("  Creating the web server on the host...done.");

  Serial.print("  Testing connection to network...");
  if (ethClient.connect(mqtt_server, 8080)) {
    Serial.println("  connected");
    ethClient.println("  GET /start/index HTTP/1.0");
    ethClient.println();
  } else {
    Serial.println("  connection failed");
  }

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

  if (mqtt_active) {

    Serial.print("  Connecting to MQTT server...");
    Serial.println(mqtt_server);
    mqttclient.setServer(mqtt_server, mqtt_port);
    Serial.println("  #define MQTT_MAX_PACKET_SIZE :  " + String(MQTT_MAX_PACKET_SIZE));

    if (mqttclient.connect(controllerName)) {
      Serial.println("  connected... Subscribing to " + String(mqtt_channel_sub));
      //sendHeartbeat();
      // mqttclient.publish(mqtt_channel_pub,"I'm alive"); 
      // ... and subscribe to topic
      mqttclient.subscribe(mqtt_channel_sub);
    } 
    Serial.print("  mqtt connected status...");
    Serial.println(mqttclient.connected());
    Serial.print("  mqtt state...");
    Serial.println(mqttclient.state());
    mqttclient.setCallback(callback);

  } else {
    Serial.print("  MQTT is disabled...Ignoring MQTT.");
  }

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
  Serial.print("Message arrived on [");
  Serial.print(topic);
  Serial.print("] topic.");

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    //return false;
  } else {
    //TODO sendState should be on the triggering cycle
    //sendState(); // shoudn't need to do this as the loop cycle checks for registry vs actual diff and publishes
    //return true;
  }

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
  String action = "";

  if (!root.success()) {
    Serial.println("ERR: parseObject() json message failed");
    return false;
  } else {
    Serial.println("Processed parseObject() successfully");
  }

  if (!root.containsKey("relay")) {
    Serial.println("ERR: no relay provided in message");
    return false;
  } else {
    relay = root["relay"];
    Serial.println("Got relay reference in process meassage of: " + String(relay));
    if (!(relay > 0 && relay < NUM_RELAYS)){
      Serial.println("ERR: no not a valid relay number [" + String(relay) + "]");
      return false;
    }
  }

  if (!root.containsKey("action") ) {
    Serial.println("ERR: no action directive");
    return false;
  } else {

    Serial.println("**** 1 ****");


    //http://arduinojson.org/faq/how-to-fix-error-ambiguous-overload-for-operator/
    action = (const char*)root["action"];
    Serial.println("**** 2 ****");
    action = root["action"].as<const char*>();
    Serial.println("**** 3 ****");
    action = root["action"].as<String>();
    Serial.println("**** 4 ****");

    Serial.println("Got action reference in process meassage of: " + String(action));

    if ((action == on_cmd) || (action == off_cmd) || (action == toggle_cmd)) {
      // if (strcmp(action, on_cmd) == 0) {
      if ((action == on_cmd)) {
        relayStates[relay] = RELAY_ON;
      }
      else if ((action == off_cmd)) {
        relayStates[relay] = RELAY_OFF;
      }
      else if ((action == toggle_cmd)) {
        relayStates[relay] = !relayStates[relay];
      }
    } else {
    Serial.print("ERR: action not recognised [");
    Serial.print(String(action));
    Serial.println("].");
    return false;      
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


  if (mqtt_active) {
    if (!mqttclient.publish(mqtt_channel_pub, buffer, true)){
      Serial.println("Sending Digital In...Failed...[message length = " + String(root.measureLength() + 1) + "]");
    } else {
      Serial.println(F("Sending Digital In...Done"));
    }    
  } else {
      Serial.println(F("Sending Digital In on MQTT...Skipped"));
      Serial.println(buffer);      
  }

}

/********************************** START SEND STATE*****************************************/
void sendState(int relay) {

  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["controller"] = controllerName;
  root["relay"] = String(relay);
  root["state"] = (relayStates[relay]) ? off_cmd : on_cmd;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  if (mqtt_active) {
    if (!mqttclient.publish(mqtt_channel_pub, buffer, true)){
      Serial.println("Sending State...Failed...[message length = " + String(root.measureLength() + 1) + "]");
    } else {
      Serial.println(F("Sending State...Done"));
    }    
  } else {
      Serial.println(F("Sending State on MQTT...Skipped"));
      Serial.println(buffer);      
  }

}

/***************************** START SEND HEARTBEAT *****************************************/
void sendHeartbeat() {

  Serial.println("Sending Heartbeat...");


  StaticJsonBuffer<BIG_BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["controller"] = controllerName;
  root["uptime_seconds"] = String(uptime);
  root["alive"] = on_cmd;

  JsonObject& relayStateJson = root.createNestedObject("relays");
  for (int i = 0; i < NUM_RELAYS; i++) 
  {
    relayStateJson[String(i)] = (relayStates[i]) ? off_cmd : on_cmd;

    //Print out we can see
    // Serial.print(" *** relay states for ");
    // Serial.print(relayStates[i]);
    // Serial.print(" :: ");
    // Serial.print((relayStates[i]) ? off_cmd : on_cmd);
    // Serial.print(" :: ");
    // Serial.print("Pin State");
    // Serial.print(" :: ");
    // Serial.println(String(digitalRead(relays[i])));
  }

  JsonObject& buttonCountJson = root.createNestedObject("buttonCounts");
  for (int k = 0; k < NUM_BUTTONS; k++) 
  {
    buttonCountJson[String(k)] = String(buttonCounters[k]);
  }

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  if (mqtt_active) {
    if (!mqttclient.publish(mqtt_heartbeat, buffer, true)){
      Serial.println("Sending Heartbeat...Failed...[message length = " + String(root.measureLength() + 1) + "]");
    } else {
      Serial.println(F("Sending Heartbeat...Done"));
    }    
  } else {
      Serial.println(F("Sending Heartbeat on MQTT...Skipped"));
      Serial.println(buffer);
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

/********************************** HELPER FUNCTIONS ****************************************/

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{
    unsigned char i;
    unsigned int  j;
    
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    for (i = 0; i < 3; i++) {
        for (j = 1; j <= 0x80; j <<= 1) {
            cl.print("<LED>");
            if ((unsigned char)relayStates[i] & j) {
                cl.print("checked");
                //Serial.println("ON");
            }
            else {
                cl.print("unchecked");
            }
            cl.println("</LED>");
        }
    }
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}



/********************************** START MAIN LOOP*****************************************/
void loop()
{

  //safely in the loop
  wdt_reset();

  if (!mqttclient.connected() && mqtt_active) {
    reconnect();
  }

  // //web server - START
  // EthernetClient webclient = webserver.available();
  //   if (webclient) {  // got client?
  //       //Seria
  //       boolean currentLineIsBlank = true;
  //       while (webclient.connected()) {
  //           if (webclient.available()) {   // client data available to read
  //               char c = webclient.read(); // read 1 byte (character) from client
  //               // limit the size of the stored received HTTP request
  //               // buffer first part of HTTP request in HTTP_req array (string)
  //               // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
  //               if (req_index < (REQ_BUF_SZ - 1)) {
  //                   HTTP_req[req_index] = c;          // save HTTP request character
  //                   req_index++;
  //               }
  //               // last line of client request is blank and ends with \n
  //               // respond to client only after last line received
  //               if (c == '\n' && currentLineIsBlank) {
  //                   // send a standard http response header
  //                   webclient.println("HTTP/1.1 200 OK");
  //                   // remainder of header follows below, depending on if
  //                   // web page or XML page is requested
  //                   // Ajax request - send XML file
  //                   if (StrContains(HTTP_req, "ajax_inputs")) {
  //                       // send rest of HTTP header
  //                       webclient.println("Content-Type: text/xml");
  //                       webclient.println("Connection: keep-alive");
  //                       webclient.println();
  //                       //SetLEDs();
  //                       // send XML file containing input states
  //                       XML_response(webclient);
  //                   }
  //                   else {  // web page request
  //                       // send rest of HTTP header
  //                       webclient.println("Content-Type: text/html");
  //                       webclient.println("Connection: keep-alive");
  //                       webclient.println();
  //                       // send web page
  //                       // webFile = SD.open("index.htm");        // open web page file
  //                       // if (webFile) {
  //                       //     while(webFile.available()) {
  //                       //         client.write(webFile.read()); // send web page to client
  //                       //     }
  //                       //     webFile.close();
  //                       // }
  //                   }
  //                   // display received HTTP request on serial port
  //                   //Serial.print(HTTP_req);
  //                   // reset buffer index and all buffer elements to 0
  //                   req_index = 0;
  //                   StrClear(HTTP_req, REQ_BUF_SZ);
  //                   break;
  //               }
  //               // every line of text received from the client ends with \r\n
  //               if (c == '\n') {
  //                   // last character on line of received text
  //                   // starting new line with next character read
  //                   currentLineIsBlank = true;
  //               } 
  //               else if (c != '\r') {
  //                   // a text character was received from client
  //                   currentLineIsBlank = false;
  //               }
  //           } // end if (client.available())
  //       } // end while (client.connected())
  //       delay(1);      // give the web browser time to receive the data
  //       webclient.stop(); // close the connection
  //   } // end if (webclient)

  // //web server - END

  for (int i = 0; i < NUM_RELAYS; i++) 
  {
    buttonStates[i] = digitalRead(buttons[i]);
    if (buttonStates[i] != buttonLastStates[i]) {
      if (buttonStates[i] == LOW) {
        buttonCounters[i]++;
        Serial.println("Button " + String(i) + " pressed for " + String(buttonCounters[i]) + " times.");
        sendDigitalIn(i);
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
  if (mqtt_active) {
    mqttclient.loop();
  }
  // Delay a little bit to avoid bouncing
  delay(50);

}
