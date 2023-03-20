/* Arduino Word Clock using NTP server
 * Local NTP Stratum 1 server: 10.69.20.5
 */
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <WDTZero.h>
#include "arduino_secrets.h"  // contains wifi SSID and Password

/*
 * Declarations for Wifi connection and NTP packets
 */
const unsigned long seventyYears = 2208988800UL;
char ssid[] = SECRET_SSID;            // network SSID
char pass[] = SECRET_PASS;            // network password
IPAddress timeServer(10, 69, 20, 5);  // local Stratum 1 NTP server
int status = WL_IDLE_STATUS;          // Status of Wifi
const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   // Buffer to hold incoming and outgoing packets
WiFiUDP Udp;                          // A UDP instance to let us send and receive packets over UDP
bool xWifiConnected = false;          // Set to TRUE if connected to Wifi 
unsigned int localPort = 2390;        // Local port to listen for UDP packets
unsigned long ntpTime;

/*
 * Declarations for the Neopixels
 */
#define NEOPIN        3       // datapin of neopixels
#define NUMPIXELS     156     // total of neopixels
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIN, NEO_GRB + NEO_KHZ800);
int individualPixels[NUMPIXELS]={0};

/*
 * System Declarations
 */
WDTZero watchDog;                     // Define WDT 
byte counter = 0;                     // counter used to update NTP/GPS sync
volatile byte state = 0;              // state of the statemachine
volatile unsigned int timerCount = 0; // used to determine 1min timer count to update the clock

/*
 * Setup Routine
 */
void setup() {
  //setupTimer();               // set the timer to regular intervals for NTP/GPS time check
  pixels.begin();             // Begin Neopixel string
  watchDog.setup(WDT_SOFTCYCLE32S);  // initialize WDT-softcounter refesh cycle on 32sec interval
  configWifi();               // Config routines for Wifi
  connectWifi();              // Make connection to Wifi bm-net
}

/*
 * Loop
 */
void loop() {
  switch(state) {
    case 1: updateDisplay(); break;
    case 2: updateSource(); break;
    case 3: updateRTC(); break;
    default: state=1; break;
  }
    
  if (counter > 30) {  // transition from state 1 -> 2 (30 x 1min = 30min)
    state = 2;
  } 
}
