/*--------------------------------------------------
Modified by m0e/04-2017
4-Relay Module controll over WiFi
Tested on WeMos D1 mini

used Pins:

5V
GND

D1  SCL
D2  SDA

D5  Relay1
D6  Relay2
D7  Relay3
D8  Relay4
----------------

HTTP 1.1 Webserver for ESP8266 
for ESP8266 adapted Arduino IDE

Stefan Thesen 04/2015

Running stable for days 
(in difference to all samples I tried)

Does HTTP 1.1 with defined connection closing.
Reconnects in case of lost WiFi.
Handles empty requests in a defined manner.
Handle requests for non-exisiting pages correctly.

This demo allows to switch two functions:
Function 1 creates serial output and toggels GPIO2
Function 2 just creates serial output.

Serial output can e.g. be used to steer an attached
Arduino, Raspberry etc.
--------------------------------------------------*/
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>

LiquidCrystal_I2C lcd(0x26,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//const char* ssid = "ESP-net";
//const char* password = "35p-r0cks-iot";

const char* ssid = "UpUpUp";
const char* password = "t4k4tuk4l4nd2";

unsigned long ulReqcount;
unsigned long ulReconncount;

const int relay1 = 14;
const int relay2 = 12;
const int relay3 = 13;
const int relay4 = 0;

// Create an instance of the server on Port 80
WiFiServer server(80);

void setup() 
{
  // setup globals
  ulReqcount=0; 
  ulReconncount=0;

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2,0);
  lcd.print("Initializing");
  lcd.setCursor(6,1);
  lcd.print("WLAN");
  delay(3000);
  
  // prepare GPIO2
  pinMode(relay4, OUTPUT);
  digitalWrite(relay4, 1);
  // prepare GPIO0
  pinMode(relay3, OUTPUT);
  digitalWrite(relay3, 1);
  // prepare GPIO4
  pinMode(relay2, OUTPUT);
  digitalWrite(relay2, 1);
  // prepare GPIO5
  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, 1);
  
  // start serial
  Serial.begin(9600);
  delay(1);
  
  // inital connect
  WiFi.mode(WIFI_STA);
  WiFiStart();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("R1: OFF");
  lcd.setCursor(9,0);
  lcd.print("R2: OFF");
  lcd.setCursor(0,1);
  lcd.print("R3: OFF");
  lcd.setCursor(9,1);
  lcd.print("R4: OFF");
}

void WiFiStart()
{
  ulReconncount++;
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() 
{
  // check if WLAN is connected
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiStart();
  }
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) 
  {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    Serial.println("client connection time-out!");
    return; 
  }
  
  // Read the first line of the request
  String sRequest = client.readStringUntil('\r');
  //Serial.println(sRequest);
  client.flush();
  
  // stop client, if request is empty
  if(sRequest=="")
  {
    Serial.println("empty request! - stopping client");
    client.stop();
    return;
  }
  
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?pin=MOTOR1STOP HTTP/1.1
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
  
  ///////////////////////////////////////////////////////////////////////////////
  // output parameters to serial, you may connect e.g. an Arduino and react on it
  ///////////////////////////////////////////////////////////////////////////////
  if(sParam.length()>0)
  {
    int iEqu=sParam.indexOf("=");
    if(iEqu>=0)
    {
      sCmd = sParam.substring(iEqu+1,sParam.length());
      Serial.println(sCmd);
    }
  }
  
  
  ///////////////////////////
  // format the html response
  ///////////////////////////
  String sResponse,sHeader;
  
  ////////////////////////////
  // 404 for non-matching path
  ////////////////////////////
  if(sPath!="/")
  {
    sResponse="<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>";
    
    sHeader  = "HTTP/1.1 404 Not found\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  ///////////////////////
  // format the html page
  ///////////////////////
  else
  {
    ulReqcount++;
    sResponse  = "<html><head><title>Demo f&uumlr ESP8266 Steuerung</title></head><body>";
    sResponse += "<font color=\"#000000\"><body bgcolor=\"#d0d0f0\">";
    sResponse += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
    sResponse += "<h1>Demo f&uumlr ESP8266 Steuerung</h1>";
    sResponse += "Schaltet Relay 1-4 per WiFi<BR>";
    sResponse += "based on ESP_WEBSERVER_HTTP11<BR>";
    sResponse += "<FONT SIZE=+1>";
    sResponse += "<p>Relay 1 <a href=\"?pin=FUNCTION1ON\"><button>einschalten</button></a>&nbsp;<a href=\"?pin=FUNCTION1OFF\"><button>ausschalten</button></a></p>";
    sResponse += "<p>Relay 2 <a href=\"?pin=FUNCTION2ON\"><button>einschalten</button></a>&nbsp;<a href=\"?pin=FUNCTION2OFF\"><button>ausschalten</button></a></p>";    
    sResponse += "<p>Relay 3 <a href=\"?pin=FUNCTION3ON\"><button>einschalten</button></a>&nbsp;<a href=\"?pin=FUNCTION3OFF\"><button>ausschalten</button></a></p>";
    sResponse += "<p>Relay 4 <a href=\"?pin=FUNCTION4ON\"><button>einschalten</button></a>&nbsp;<a href=\"?pin=FUNCTION4OFF\"><button>ausschalten</button></a></p>";
    
    //////////////////////
    // react on parameters
    //////////////////////
    if (sCmd.length()>0)
    {
      // write received command to html page
      sResponse += "Kommando:" + sCmd + "<BR>";
      
      // switch GPIO 2
      if(sCmd.indexOf("FUNCTION1ON")>=0)
      {
        digitalWrite(relay1, 0);
        lcd.setCursor(0,0);
        lcd.print("R1: ON ");
      }
      else if(sCmd.indexOf("FUNCTION1OFF")>=0)
      {
        digitalWrite(relay1, 1);
        lcd.setCursor(0,0);
        lcd.print("R1: OFF");
      }
            // switch GPIO 0
      if(sCmd.indexOf("FUNCTION2ON")>=0)
      {
        digitalWrite(relay2, 0);
        lcd.setCursor(9,0);
        lcd.print("R2: ON ");        
      }
      else if(sCmd.indexOf("FUNCTION2OFF")>=0)
      {
        digitalWrite(relay2, 1);
        lcd.setCursor(9,0);
        lcd.print("R2: OFF");
      }
                  // switch GPIO 4
      if(sCmd.indexOf("FUNCTION3ON")>=0)
      {
        digitalWrite(relay3, 0);
        lcd.setCursor(0,1);
        lcd.print("R3: ON ");
      }
      else if(sCmd.indexOf("FUNCTION3OFF")>=0)
      {
        digitalWrite(relay3, 1);
        lcd.setCursor(0,1);
        lcd.print("R3: OFF");
      }
                  // switch GPIO 5
      if(sCmd.indexOf("FUNCTION4ON")>=0)
      {
        digitalWrite(relay4, 0);
        lcd.setCursor(9,1);
        lcd.print("R4: ON ");
      }
      else if(sCmd.indexOf("FUNCTION4OFF")>=0)
      {
        digitalWrite(relay4, 1);
        lcd.setCursor(9,1);
        lcd.print("R4: OFF");
      }
    }
    
    sResponse += "<FONT SIZE=-2>";
    sResponse += "<BR>Aufrufz&auml;hler="; 
    sResponse += ulReqcount;
    sResponse += " - Verbindungsz&auml;hler="; 
    sResponse += ulReconncount;
    sResponse += "<BR>";
    sResponse += "m0e-2017 original by Stefan Thesen 04/2015<BR>";
    sResponse += "</body></html>";
    
    sHeader  = "HTTP/1.1 200 OK\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  
  // Send the response to the client
  client.print(sHeader);
  client.print(sResponse);
  
  // and stop the client
  client.stop();
  Serial.println("Client disonnected");
}
