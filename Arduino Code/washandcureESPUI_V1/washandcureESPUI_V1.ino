//ToDO
//1. AP that can search for networks to connect to
//2. Change wash direction mid term


// include library to read and write from flash memory
#include <EEPROM.h>
#include <Bounce2.h>
#include <DNSServer.h>
#include <ESPUI.h>

//define Hotspot IP and DNS port
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;

#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

//define your wi-fi network credentials
const char* ssid     = "W&C";
const char* password = "washandcure";



//define AP hostname
const char *hostname = "wash&cure";


// define the number of bytes we want to access in the EEPROM to save the Wash and cure time
#define EEPROM_SIZE 2
int WashValue;   // store the curing and wash timer value
int CureValue;   // store the curing and wash timer value


// Include the AccelStepper library:
#include <AccelStepper.h>
// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
#define dirPin 25 // driver stick DIR pin
#define stepPin 33 // // driver stick STP pin
#define motorInterfaceType 1
#define motorEnable 26 // driver stick VDD pin
#define UvPin  32    //UV Pin
#define FanPin  27     //Fan  pin 
#define SW1  35     //SW1 Button 
#define SW2  34     //SW2 Button 
#define SW3  0      //SW3 Button . Used for programming mode also .
int IRInputPin = 14; //Proximity Sensor for the LID
int IROutputState;  //Proximity Sensor state

//enable Bounce on the 3 GPIO pins: SW1,SW2,DWN
Bounce debouncer1 = Bounce(); 
Bounce debouncer2 = Bounce(); 
Bounce debouncer3 = Bounce(); 

//OLED libraries and settings
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);



int statusLabelId;
int millisLabelId;
int WashSwitchId;
int UvSwitchId;
static bool WashSwitchState = false;
static bool UvSwitchState = false;

//Timer variables

// for Washing Cycle
int washSeconds ;
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean washTimer = false;

// for UV Curing Cycle
int uvSeconds ;
boolean uvTimer = false;

//add slider for timing
void slider(Control *sender, int type) {
  Serial.print("Slider: ID: ");
  Serial.print(sender->id);
  Serial.print(", Value: ");
  Serial.println(sender->value);
  // Like all Control Values in ESPUI slider values are Strings. To use them as int simply do this:
  int WashValueWithOffset = sender->value.toInt();
  // save the timer value in flash memory
    EEPROM.write(0, WashValueWithOffset);
    EEPROM.commit();
    Serial.println("level saved in flash memory");
    WashValue = EEPROM.read(0);
    Serial.println("Wash value from memory");
    Serial.println(WashValue);
    washSeconds = WashValue*60 ;
}

void slider2(Control *sender, int type) {
  Serial.print("Slider: ID: ");
  Serial.print(sender->id);
  Serial.print(", Value: ");
  Serial.println(sender->value);
  // Like all Control Values in ESPUI slider values are Strings. To use them as int simply do this:
  int CureValueWithOffset = sender->value.toInt();
  // save the timer value in flash memory
    EEPROM.write(1, CureValueWithOffset);
    EEPROM.commit();
    Serial.println("level saved in flash memory");
    CureValue = EEPROM.read(1);
    Serial.println("Cure value from memory");
    Serial.println(CureValue);
    uvSeconds = CureValue*60 ;
}


void numberCall(Control *sender, int type) { Serial.println(sender->value); }

void switchExample(Control *sender, int value) {
  switch (value) {
  case S_ACTIVE:
  //WashSwitchState = true;
    Serial.print("Active:");
     ESPUI.print(statusLabelId, "Wash Active");
   wash();
    break;

  case S_INACTIVE:
 // WashSwitchState = false;
    Serial.print("Inactive");
     ESPUI.print(statusLabelId, "Ready");
    washoff();
    break;
  }

  Serial.print(" ");
  Serial.println(sender->id);
}

void otherSwitchExample(Control *sender, int value) {
  switch (value) {
  case S_ACTIVE:
    Serial.print("Active:");
    ESPUI.print(statusLabelId, "Cure Active");
    cure();
    break;

  case S_INACTIVE:
    Serial.print("Inactive");
    ESPUI.print(statusLabelId, "Ready");
    cureoff();
    break;
  }

  Serial.print(" ");
  Serial.println(sender->id);
}


void wash( ) {
  Serial.println("Wash Cycle ON !");
   WashValue = EEPROM.read(0);
    Serial.println("Wash value from memory");
    Serial.println(WashValue);
    washSeconds = WashValue*60 ;
   //WashSwitchState = true;
   washTimer = true; // start the washing timer
   lastTrigger = millis(); 
   digitalWrite(FanPin, HIGH); //turn on the fan
   digitalWrite(motorEnable, HIGH); // enable the motor
   
   stepper.setSpeed(16000); // set the motor speed
   stepper.runSpeed(); // start the motor

   //Handle the OLED screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Washing");
  display.display(); 
   
  return;
  }
 void washoff(){
  
   ESPUI.print(statusLabelId, "Ready");
  digitalWrite(FanPin, LOW); // turn off the fan
  digitalWrite(motorEnable, LOW); // turn off the motor
  
   WashSwitchState = false;
   ESPUI.updateSwitcher(WashSwitchId, WashSwitchState);

  washTimer = false; // stop the timer
  Serial.println("Wash Cycle Finished!Returning to main loop");
  //handle the OLED screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Washing");
  display.print(" Done !");
  display.display(); 
  return;

}

void cure()
{ 
  
   Serial.println("UV Cycle ON !");
    CureValue = EEPROM.read(1);
    Serial.println("Cure value from memory");
    Serial.println(CureValue);
    uvSeconds = CureValue*60 ;
   uvTimer = true; // start the UV timer
   lastTrigger = millis();
   
  digitalWrite(motorEnable, HIGH); // turn on the motor
  digitalWrite(FanPin, HIGH); // turn on the fan
  digitalWrite(UvPin, HIGH); // turn on the UV lamp
  stepper.setSpeed(500); // set the motor speed
  stepper.runSpeed(); // start the motor
  
  //handle the OLED screen
  display.clearDisplay();
  delay(5);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(1, 10);
  
  // Display static text
  display.println("UV Curing");
  display.display(); 
  // ESP.wdtFeed();
 


}
void cureoff(){
UvSwitchState = false;
ESPUI.updateSwitcher(UvSwitchId, UvSwitchState);
ESPUI.print(statusLabelId, "Ready");
digitalWrite(UvPin, LOW); // turn off UV
digitalWrite(FanPin, LOW);  // turn off the fan
digitalWrite(motorEnable, LOW); // turn off the motor

uvTimer = false; // stop the UV timer

Serial.println("UV Cycle Finished!Returning to main loop");
// handle the oled screen
  display.clearDisplay();
  delay(5);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  
  // Display static text
  display.println("Curing ");
  display.print(" Done !");
  display.display(); 
return;
}

void StopAll() {
cureoff();
washoff();

Serial.println("The lid is not set !");
// handle the oled screen
  display.clearDisplay();
  delay(5);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  
  // Display static text
  display.println("LID!");
 //display.print(" Done !");
  display.display(); 
return;

}


void setup(void) {
    // initialize EEPROM with predefined size
     EEPROM.begin(EEPROM_SIZE);
     WashValue = EEPROM.read(0);
     CureValue = EEPROM.read(1);
  Serial.begin(115200);

  // define the GPIO functions
  pinMode(IRInputPin, INPUT);
  pinMode(motorEnable, OUTPUT);
  pinMode(UvPin, OUTPUT);
  pinMode(FanPin, OUTPUT);
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);      
  pinMode(SW3, INPUT);     
  //define the GPIO initial states
  digitalWrite(motorEnable, LOW);
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  digitalWrite(UvPin, LOW); // NEVER SET THIS HIGH ! IT WILL DAMAGE THE MOSFET AND YOU WILL HAVE TO REPLACE IT !
  digitalWrite(FanPin, LOW); //NEVER SET THIS HIGH ! IT WILL DAMAGE THE MOSFET AND YOU WILL HAVE TO REPLACE IT !
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  debouncer1.attach(SW1);
  debouncer1.interval(5); // interval in ms
  debouncer2.attach(SW2);
  debouncer2.interval(5); // interval in ms
  debouncer3.attach(SW3);
  debouncer3.interval(5); // interval in ms
  
  stepper.setMaxSpeed(16000); // set motor max speed
  stepper.setAcceleration(100); // set motor acceleration
  
  ///OLED setup
  Wire.begin(21,22);//OLED I2C Pins
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address for the I2C OLED screen
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  
  // Display static text
  display.println("Powering ");
  display.print(" UP ");
  display.display(); 
  ESPUI.setVerbosity(Verbosity::VerboseJSON);



#if defined(ESP32)
  WiFi.setHostname(hostname);
#else
  WiFi.hostname(hostname);
#endif

  // try to connect to existing network
  WiFi.begin(ssid, password);
  Serial.print("\n\nTry to connect to existing network");

  {
    uint8_t timeout = 10;

    // Wait for connection, 5s timeout
    do {
      delay(500);
      Serial.print(".");
      timeout--;
    } while (timeout && WiFi.status() != WL_CONNECTED);

    // not connected -> create hotspot
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("\n\nCreating hotspot");

      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP(ssid);

      timeout = 5;

      do {
        delay(500);
        Serial.print(".");
        timeout--;
      } while (timeout);
    }
  }

  dnsServer.start(DNS_PORT, "*", apIP);

  Serial.println("\n\nWiFi parameters:");
  Serial.print("Mode: ");
  Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
  Serial.print("IP address: ");
  Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
  //display Wi-Fi mode and IP address
  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  
  // Display static text
  display.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
  display.setTextSize(1);
  display.print("IP: ");
  display.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
  display.display(); 
  
  statusLabelId = ESPUI.label("Status:", ControlColor::Sunflower, "Ready");
  WashSwitchId = ESPUI.switcher("Wash", &switchExample, ControlColor::Peterriver, false);
  ESPUI.slider("Wash Duration (minutes): ", &slider, ControlColor::Peterriver, WashValue, 1,15);
  UvSwitchId = ESPUI.switcher("Cure", &otherSwitchExample, ControlColor::Emerald, false);
  ESPUI.slider("Cure Duration (minutes): ", &slider2, ControlColor::Emerald, CureValue, 1,15);


  /*
   * .begin loads and serves all files from PROGMEM directly.
   * If you want to serve the files from SPIFFS use ESPUI.beginSPIFFS
   * (.prepareFileSystem has to be run in an empty sketch before)
   */
 
  // Enable this option if you want sliders to be continuous (update during move) and not discrete (update on stop)
  // ESPUI.sliderContinuous = true;

  /*
   * Optionally you can use HTTP BasicAuth. Keep in mind that this is NOT a
   * SECURE way of limiting access.
   * Anyone who is able to sniff traffic will be able to intercept your password
   * since it is transmitted in cleartext. Just add a string as username and
   * password, for example begin("ESPUI Control", "username", "password")
   */
  ESPUI.begin("Wash and Cure Station");

}

void loop(void) {
  dnsServer.processNextRequest();
  now = millis();

IROutputState = digitalRead(IRInputPin);
  //Check if the lid is on the wash and cure machine at all times. If the lid is not present or removed, stop the machine immediately. 
  if ( IROutputState == HIGH && washTimer == true || IROutputState == HIGH && uvTimer == true) {
    
    StopAll();
  }
else {
 
}

  
  if(washTimer == true && (now - lastTrigger > (washSeconds*1000))) {
  Serial.println("Washing stopped by timer…");
  washoff();
  }
    if(uvTimer == true && (now - lastTrigger > (uvSeconds*1000))) {
  Serial.println("UV Cure stopped by timer…");
  cureoff();

}
  // Update the Bounce instances :
  debouncer1.update();
  debouncer2.update();
  debouncer3.update(); 
  int value1 = debouncer1.read();
  int value2 = debouncer2.read();
  int value3 = debouncer3.read();
  


if (debouncer1.fell() && washTimer == false && uvTimer == false)
{
  cure();
}
else if( debouncer1.fell() && washTimer == true && uvTimer == false )
{
washoff();
delay(1);
//cure();
}
else if( debouncer1.fell() && washTimer == false && uvTimer == true )
{
cureoff();
delay(1);
}
else
{
  // action B
}

if (debouncer2.fell() && uvTimer == false && washTimer == false  )
{
  wash();
}
else if( debouncer2.fell() && uvTimer == true && washTimer == false  )
{
cureoff();
delay(1);
//wash();
}
else if( debouncer2.fell() && uvTimer == false && washTimer == true  )
{

washoff();
}
else
{
  // action B
}
if (debouncer3.fell() && washTimer == true && uvTimer == false)
{
washoff();
}
else if( debouncer3.fell() && washTimer == false && uvTimer == true )
{
cureoff();
delay(1);
//cure();
}
else if( debouncer3.fell() && washTimer == false && uvTimer == false ) // display the network status
{
display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  
  // Display static text
  display.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
  display.setTextSize(1);
  display.print("IP: ");
  display.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
  display.display(); 
}
else
{
  // action B
}
stepper.runSpeed();
}
