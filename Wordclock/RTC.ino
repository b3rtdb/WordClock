/*
 * Main routine for ntp update
 * STATE = 2
 */
void updateSource() {
  if (xWifiConnected) {
    ntpTime = getNTPTime();
    if (ntpTime != 0) {
      state = 3;
    }
    state = 98;  // report error / NTP get time failed
  }
  else state = 99; // report error / we are not connected to the wifi
}

/*
 * Main routine for RTC update to DS1307
 * STATE = 3
 */
void updateRTC() {
  
}
