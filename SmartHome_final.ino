#include <ESP8266WiFi.h> //WiFi Module
#include "dhtStable.h" //Humidity & Temperature sensor DHT11
#include <WiFiClientSecure.h> //
#include <UniversalTelegramBot.h> //Telgram Bot
#include <ArduinoJson.h> 


// WiFi Credentials
const char* ssid = "Saketh Ram";
const char* password = "saketh03";
WiFiServer server(80);  // Set port to 80

// Initialize Telegram BOT
#define BOTtoken "6198376034:AAG38M-xUgGxCZ18YE3KQOR0hej6lm975oo"  // your Bot Token (Get from Botfather)
#define CHAT_ID "5104084095"

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);


//Declaring pins
int relay1 = D1; //for LED LIGHT control
int relay2 = D2; //for Sensor Based control 
#define DHT11_PIN D3 //for DHT11 sensor
const int motionSensor = D4; // PIR Motion Sensor


DHTStable DHT; //DHTStable class from dhyStable.h

String header;  // This stores the HTTP request

//Declaring states for each appliance

String automode = "off";   //operation of AC - Auto/Manual Mode
String relay1state = "off";  // State of Relay1
String relay2state = "off";  // State of Relay2
String pir_status = "off";

bool motionDetected = false;

// Function when motion is detected
void ICACHE_RAM_ATTR detectsMovement() {
  Serial.println("MOTION DETECTED!!!");
  motionDetected = true;
}

void setup() {
  Serial.begin(115200);

  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org


  pinMode(relay1,OUTPUT);  // declaring Relay1 as output
  pinMode(relay2,OUTPUT); 
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);

  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);


  //connect to access point
  WiFi.begin(ssid, password);
  Serial.println("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  // this will display the Ip address of the Pi which should be entered into your browser
  server.begin();
  
}

void loop() {
  WiFiClient client = server.available();  // Listen for incoming clients

  int readData = DHT.read11(DHT11_PIN);
 
  float humidityval =DHT.getHumidity();
  float temperatureval = DHT.getTemperature();

  if (client) {                   // If a new client connects,
    String currentLine = "";      // make a String to hold incoming data from the client
    while (client.connected()) {  // loop while the client's connected
      if (client.available()) {   // if there's bytes to read from the client,
        char c = client.read();   // read a byte, then
        Serial.write(c);          // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /relay1/on") >= 0) {
              Serial.println("Relay1 on");
              relay1state = "on";
              digitalWrite(relay1, HIGH);
            } else if (header.indexOf("GET /relay1/off") >= 0) {
              Serial.println("Relay1 off");
              relay1state = "off";
              digitalWrite(relay1, LOW);
            } else if (header.indexOf("GET /automode/on") >= 0) {
              Serial.println("Auto Mode ON");
              automode = "on";
              if (humidityval >= 30 && temperatureval >= 30) {
                Serial.println("Relay2 on");
                relay2state = "on";
                digitalWrite(relay2, HIGH);
              } else {
                Serial.println("Relay2 off");
                relay2state = "off";
                digitalWrite(relay2, LOW);             
              }
            } else if (header.indexOf("GET /automode/off") >= 0) {
              Serial.println("Auto Mode OFF");
              automode = "off";
              if (header.indexOf("GET /automode/off/relay2/on") >= 0) {
                Serial.println("Relay2 on");
                relay2state = "on";
                digitalWrite(relay2, HIGH);
              } else if (header.indexOf("GET /automode/off/relay2/off") >= 0) {
                Serial.println("Relay2 off");
                relay2state = "off";
                digitalWrite(relay2, LOW);
              }
            } else if (header.indexOf("GET /pir_sensor/on") >= 0) {
              Serial.println("Montion Detection on");
              pir_status = "on";
              //digitalWrite(relay1, HIGH);
            } else if (header.indexOf("GET /pir_sensor/off") >= 0) {
              Serial.println("Motion Detection off");
              pir_status = "off";
              //digitalWrite(relay1, LOW);
            }           
            // Display the HTML web page
            client.println("<!DOCTYPE html><html lang=\"en\">");
            client.println("<head><meta charset=\"UTF-8\"/><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"/>");
            client.println("<meta name=\"viewport\" content=\"width=device-width,initial scale=1.0\"/>");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<title>House Controls</title>");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>body{background-color: #c5e8fa;}html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 10px 30px;");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".light{background-color: #87CEFA; width: 300px; border: 7px solid black;padding: 20px;margin: 10px;justify-content:center;height:180px;}");
            client.println(".AC{background-color: #87CEFA;width: 300px;border: 7px solid black;padding: 20px;margin: 10px;}");
            client.println(".pir{background-color: #87CEFA;width: 300px;border: 7px solid black;padding: 20px;margin: 10px;height:180px;}");
            client.println(".temp{background-color:white;width: 200px;border: 5px solid black;padding: 5px;margin: 10px;position:relative;justify-content:center;left:10%;right:50%}");
            client.println(".button2 {background-color: #77878A;}");
            client.println(".flex_container{display: flex;justify-content: center;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>SMART HOME AUTOMATION SYSTEM</h1>");
            client.println("<body><h2>Welcome, User!!</h2>");

            // Display current state, and ON/OFF buttons for GPIO 5
            client.println("<div class=\"flex_container\">");
            client.println("<div class=\"light\">");
            client.println("<h3>LIGHTING CONTROL &#128161</h3>");
            client.println("<p>Lights Status- " + relay1state + "</p>");
            // If Appliance 1 is off, it displays the ON button
            if (relay1state == "off") {
              client.println("<p><a href=\"/relay1/on\"><button class=\"button\">Switch ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/relay1/off\"><button class=\"button button2\">Switch OFF</button></a></p>");
            }
            client.println("</div>");
            // Display current state, and ON/OFF buttons for GPIO 4
            client.println("<div class=\"AC\">");
            client.println("<h3>APPLIANCES CONTROL</h3>");
            client.println("<p>Air Conditioner (AC) - " + relay2state + "</p>");
            if(automode=="on")
              client.println("<p>Operating Mode - Auto</p>");
            else if(automode=="off")
              client.println("<p>Operating Mode - Manual</p>");
            //ADD SENSOR STATUS
            client.println("<div class=\"temp\">");
            client.println("<h4>SENSOR STATUS</h4>");
            client.println("<p>Humidity: ");
            client.println((float)humidityval);
            client.println("%</p>");
            client.println("<p>Temperature: ");
            client.println((float)temperatureval);
            client.println(" &#176C</p>");
            client.println("</div>");

            // If Appliance 2 is off, it displays the ON button
            if (automode == "on") {
              client.println("<p><a href=\"/automode/off\"><button class=\" button buttonp\">Switch to MANUAL Mode</button></a></p>");
            } else {
              if (relay2state == "off") {
                client.println("<p><a href=\"/automode/off/relay2/on\"><button class=\"button\">Switch ON</button></a></p>");
              } else {
                client.println("<p><a href=\"/automode/off/relay2/off\"><button class=\"button button2\">Switch OFF</button></a></p>");
              }
              client.println("<p><a href=\"/automode/on\"><button class=\" button buttonp\">Switch to AUTO Mode</button></a></p>");
            }
            client.println("</div>");

            client.println("<div class=\"pir\">");
            client.println("<h3>SECURITY SYSTEM</h3>");
            client.println("<p>Motion Detection - " + pir_status + "</p>");
            if (pir_status == "off") {
              client.println("<p><a href=\"/pir_sensor/on\"><button class=\"button\">Turn ON</button></a></p>");
               bot.sendMessage(CHAT_ID, "Bot stopped", "");
            } 
            else {
              bot.sendMessage(CHAT_ID, "Bot started up", "");
              //ICACHE_RAM_ATTR detectsMovement()
              
                if(motionDetected)
                {
                  bot.sendMessage(CHAT_ID, "Motion detected!!", "");
                  Serial.println("Motion Detected");
                  motionDetected = false;
                }
              client.println("<p><a href=\"/pir_sensor/off\"><button class=\"button button2\">Turn OFF</button></a></p>");
            }
            client.println("</div");
            client.println("</div>");
            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println(""); 
  }
  
}
