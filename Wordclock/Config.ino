/*
 * Config the timer interrupt
 */
void setupTimer() {
// get timer commando's
}

/*
 * Timer Interrupt Routine
 */
void TimerIRQ() {
  watchDog.clear();               // refresh wdt
  timerCount++;
  if(timerCount == 120)  {        // Timer interrupt every 1 minute
    state = 1;                    // transition from state 2 -> 1
    timerCount = 0;
  }
}

/*
 * Config routines for the wifi module
 */
void configWifi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // FALLBACK TO GPS NECESSARY
  }
  
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
}

/*
 * Make connection to Wifi and open UDP socket
 */
void connectWifi() {
  while (status != WL_CONNECTED) {
    xWifiConnected = false;
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);       // wait 10 seconds for connection
  }
  Serial.println("Connected to wifi");
  xWifiConnected = true;
  printWifiStatus();
  Udp.begin(localPort); // start connection
}

/*
 * Print the Wifi Status - only for debugging
 */
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
