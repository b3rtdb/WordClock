/*
 * Get NTP time, return an unsigned long with seconds sinds 1 jan 1900
 * The timestamp starts at byte 40 of the received packet and is four bytes,
 * or two words, long. First, extract the two words.
 * Then, combine the four bytes (two words) into a long integer,
 * this is the NTP time (seconds since Jan 1 1900).
 * Subtract 70 years to get unix time.
 */
unsigned long getNTPTime() {
  sendNTPpacket(timeServer);  // send an NTP packet to the time server
  delay(1000);                // wait to see if a reply is available
  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    unsigned long epoch = secsSince1900 - seventyYears;
    return epoch;
  }
}

/*
 * send an NTP request to the time server at the given address
 */
unsigned long sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
