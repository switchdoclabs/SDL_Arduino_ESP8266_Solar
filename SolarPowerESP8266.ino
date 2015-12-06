
/*
    SwitchDoc Labs Code for SolarPower ESP8266

    Uses ESP8266 and SunAirPlus

    Dedember 2015

*/

#pragma GCC diagnostic ignored "-Wwrite-strings"

extern "C" {
#include "user_interface.h"
}
#include <ESP8266WiFi.h>

#define DEBUG_MODE 1

#include <aREST.h>

#include <String.h>



// SunAirPlus Data Structures
// Note:  SunAirPlus uses a 3 channel current/voltage I2C chip - INA3221 to read all values - see github.com/switchdoclabs/SDL_Arduino_INA3221
//

#include "SDL_Arduino_INA3221.h"

// SAP INA3221
SDL_Arduino_INA3221 ina3221_SAP;



// structure for one SAP ina3221
struct SAPData
{

  float busVoltage[3];
  float current[3];
  float loadVoltage[3];

};



SAPData currentSAPData;

// the three channels of the INA3221 named for SunAirPlus Solar Power Controller channels (www.switchdoc.com)
#define SAP_LIPO_BATTERY_CHANNEL 0
#define SAP_SOLAR_CELL_CHANNEL 1
#define SAP_OUTPUT_CHANNEL 2

// SAP Buffer for sending readings to RaspberryPi

struct SAPBufferStruct
{

  unsigned long timeStamp;
  SAPData SAPEntry;


};

#define SAPBUFFERSIZE 200

SAPBufferStruct SAPBuffer[SAPBUFFERSIZE];

int CurrentSAPBuffer;

int lastReadSAPBuffer;

#include "SAPData.h"


//----------------------------------------------------------------------
//Local WiFi SunAirPlus

const char* ssid = "YOURSSID";
const char* password = "YOURPASSWORD";

#define SOLARPOWERESP8266VERSION 004



//----------------------------------------------------------------------


int blinkPin = 0;                // pin to blink led at each reading




// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

unsigned long oldReadSunAirPlusTime;
unsigned long newReadSunAirPlusDeltaTime;

int RestTimeStamp;
String RestDataString;


// Create aREST instance
aREST rest = aREST();

// Custom function accessible by the API
int ledControl(String command) {

  // Get state from command
  int state = command.toInt();

  digitalWrite(0, state);
  return 1;
}

void setup() {


  pinMode(blinkPin, OUTPUT);        // pin that will blink every reading
  digitalWrite(blinkPin, HIGH);  // High of this pin is LED OFF

  // SAP initialization

  startSAPINA3221();

  Serial.begin(115200);             // we agree to talk fast!


  Serial.println("----------------");
  Serial.println("SolarPower ESP8266");
  Serial.println("----------------");




  RestTimeStamp = 0;
  RestDataString = "";

  rest.variable("RestTimeStamp", &RestTimeStamp);
  rest.variable("RestDataString", &RestDataString);

  // Function to be exposed
  rest.function("led", ledControl);

  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("SolarPowerESP8266");


  Serial.print("Connecting to ");
  Serial.print(ssid);
  if (strcmp (WiFi.SSID().c_str(), ssid) != 0) {
    WiFi.begin(ssid, password);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.print("Local WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();
  Serial.println("Server started");

  oldReadSunAirPlusTime = micros();

  initSAPBuffer();


}

int sampleCount = 0;

// Loop through reading current and solar performance

void loop() {



  // Handle REST calls
  WiFiClient client = server.available();
  if (client)
  {

    while (!client.available()) {
      delay(1);
    }
    if (client.available())
    {
      Serial.print("Buffer Count=");
      Serial.println(returnCountSAPBuffer());
      RestTimeStamp = millis();
      //printDebugFullSAPBuffer();
      RestDataString = assembleSAPBuffer();
      rest.handle(client);
    }
  }

  newReadSunAirPlusDeltaTime = micros() - oldReadSunAirPlusTime; // doing this handles the 71 second rollover because of unsighned arithmetic 


  if (newReadSunAirPlusDeltaTime > 1000000)  // check for 1 second work to be done
  {

    Serial.print("Free heap on ESP8266:");
    Serial.println(ESP.getFreeHeap(), DEC);

    digitalWrite(blinkPin, LOW);  // High of this pin is LED ON
    Serial.println();
    readSAP();
    writeSAPBuffer();

    digitalWrite(blinkPin, HIGH);  // High of this pin is LED OFF

    oldReadSunAirPlusTime = micros();



    //printDebugFullSAPBuffer();
  }


  yield(); //  take a break
}




