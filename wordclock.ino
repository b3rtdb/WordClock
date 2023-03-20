#include "WordClock.h"
//#include <SPI.h>

#define DEBUG_LED 2

// MAX7219
#include "LedControl.h"

// touch buttons
#include "TouchSensor.h"

// RTC
#include <Wire.h>
#include "RTClib.h"

#define DEBUG
#ifdef DEBUG
#define debug(str) Serial.println(str)
#define debug2(str1, str2) Serial.print(str1); Serial.println(str2);
#else
#define debug(str)
#define debug2(str1, str2)
#endif


/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */

LedControl lc;
TouchSensor touchSensor;
RTC_DS1307 RTC;
RTC_Millis RTC_Millis;



byte x;


/* we always wait a bit between updates of the display */
unsigned long delaytime=250;

void setup() 
{
#ifdef DEBUG
  Serial.begin(9600);
#endif
  debug(F("Setup()"));
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(DEBUG_LED, LOW);

  lc.attach(5, 6, 7);// data, clock, latch;
  lc.shutdown(false);
  lc.setIntensity(15);// max brightness
  
  touchSensor.attach();

  Wire.begin();
  RTC.begin();
  RTC.setSquarewave(true);

  // set current time (run only once
  //RTC.adjust(DateTime(__DATE__, __TIME__)); //last adjustment 2013-12-11 11:27:00 (1386761220)
  // @2013-12-17 11:57:04 (1387281424) off by 153second (so 153s ahead off after 520204s => 1second every 3400.026 seconds

  unsigned long startTime = millis();
  DateTime now = RTC.now();
  Serial.print(millis()-startTime);
  Serial.println();
  
  
  RTC_Millis.adjust(now);
  now = RTC_Millis.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();


  // fade in current rtc time
  lc.setIntensity(0);
  showCurrentTime();
  delay(16);// wait for one 60Hz cycle
  fadeIntensity(0, 15, 500);    

}




void loop() 
{ 
  while( true )
  {
    //touchSensor.logTimes(true);  
    uint8_t buttonsPressed = touchSensor.getButtonsPressed();

    if( buttonsPressed==(BUTTON1|BUTTON2) ) //both middle buttons pressed
    {
      lc.flashStatusLeds();
      unsigned long pressStart=millis();
      
      //check if pressed for at least 200ms
      do
      {
        lc.flashStatusLeds();
        delay(50);
      }
      while( (millis()-pressStart)<200 && touchSensor.getButtonsPressed()==(BUTTON1|BUTTON2) );
      
      // wait till all released
      while( touchSensor.getButtonsPressed()!=0 )
      {
        lc.flashStatusLeds();
        delay(50);
      }
      
      // check if presses for no more than 800ms (to prevent false positive presses even more)
      if( (millis()-pressStart)>2000 )
      {
        continue;
      }

      // enter time change mode      
      alterTimeMode();

    }
    else if( buttonsPressed==(BUTTON0) ) //1st button pressed
    {
      tempMode();
    }
    else if( buttonsPressed==(BUTTON0|BUTTON3) ) //both outside buttons pressed
    {
      demoMode();
    }
    
    showCurrentTime(true);
    adjustBrightness();
    delay(50);

  }//end while
 
}

#define MAXBRIGHTNESS 600
#define MINBRIGHTNESS 100
void adjustBrightness()
{
  static byte oldIntensity = 15;
  
  byte intensity = min(15, max(0, 15*(analogRead(A3)-MINBRIGHTNESS)/(MAXBRIGHTNESS-MINBRIGHTNESS)));
  lc.setIntensity(intensity);
  
  if( oldIntensity!=intensity )
  {
    Serial.print("intensity: ");
    Serial.print(intensity);
    oldIntensity = intensity;
  }
  //Serial.println(analogRead(A3));
  /*
  if (Serial.available() > 0) {
      // read the incoming byte:
      byte b = Serial.read();
      byte val = b<='9' ? b-'0' : 10+b-'a';
      lc.setIntensity(val);
      Serial.println(val);
    }
//    Serial.println();
  */
}


void alterTimeMode()
{
  do
  {
    /*
     * set hours
     */
    int8_t hours = alterTime(HOURS);
    if( hours==-1 )
    {
      showCurrentTime();
      break;
    }
    
    /*
     * set minutes
     */
    int8_t minutes = alterTime(MINUTES);
    if( minutes==-1 )
    {
      showCurrentTime();
      break;
    }
  
    /*
     * set minutes
     */
    int8_t seconds = alterTime(SECONDS);
    if( seconds==-1 )
    {
      showCurrentTime();
      break;
    }
  
    /* 
     * Save new values
     */      
    lc.setChars("OK", 2);
    while( touchSensor.getButtonsPressed()!=0 )
    {
      lc.flashStatusLeds();
      delay(50);
    }
    delay(1000);
   
  } while (false);
    
  showCurrentTime();  
  // wait for all buttons to be released
  while( touchSensor.getButtonsPressed()!=0 )
  {
    lc.flashStatusLeds();
    delay(50);
  }  

}


int8_t alterTime(changevalue option)
{
  byte value;
  int8_t diff = 0;

  char text[16];
  byte textLength;
  byte maxValue;
  DateTime now;
  switch( option )
  {
    case HOURS:
      textLength = 3;
      strncpy(text, "Uur", textLength);
      maxValue = 24;
      break;
    case MINUTES:
      textLength = 7;
      strncpy(text, "Minuten", textLength);
      maxValue = 60;
      break;
    case SECONDS:
      textLength = 8;
      strncpy(text, "Seconden", textLength);
      maxValue = 60;
      break;
    default:
      return -1;
  }
  
  /* show ticker */
  lc.setTicker(text, textLength, false);
  // if any button was before this function was called pressed; wait till released (note that we show ticker first, to have a responsive UI)
  while( touchSensor.getButtonsPressed()!=0 )
  {
    lc.flashStatusLeds();
    delay(50);
  }
  //show ticker till finished, or a button was pressed
  while( !lc.isTickerDone() && touchSensor.getButtonsPressed()==0 )
    delay(50);
  lc.stopTicker();
  
  /* show number */
  now = RTC_Millis.now();
  switch( option )
  {
    case HOURS:
      value = now.hour();
      break;
    case MINUTES:
      value = now.minute();    
      break;
    case SECONDS:
      value = now.second();
      break;
  }
  lc.setNumber((value+diff) % maxValue);

  // if button was pressed; wait till released
  while( touchSensor.getButtonsPressed()!=0 )
  {
    lc.flashStatusLeds();
    delay(50);
  }
  
  while( true )
  {
    now = RTC_Millis.now();
    switch( option )
    {
      case HOURS:   value = now.hour(); break;
      case MINUTES: value = now.minute(); break;
      case SECONDS: value = now.second(); break;
    }
    lc.setNumber((value+diff) % maxValue);

    uint8_t buttonsPressed = touchSensor.getButtonsPressed();
    if( buttonsPressed==BUTTON0 )
    {
      /*
      for( byte row=0; row<MAXROWS; row++)
        for( byte col=0; col<MAXCOLS; col++)
          lc.setLed(row, col, ON);
      delay(1000);
      lc.clearDisplay(false);
      delay(1500);
      */
      lc.flashStatusLeds();
      return -1; // exit
    } 
    else if( buttonsPressed==BUTTON3 )
    {
      lc.flashStatusLeds();
      return (value+diff) % maxValue; // new value selected
    }
    else if ( buttonsPressed==BUTTON1 || buttonsPressed==BUTTON2 )
    {
      lc.flashStatusLeds();
      if( buttonsPressed==BUTTON1 )
        diff = (diff+maxValue-1) % maxValue;
      else
        diff = (diff+1) % maxValue;

      //DateTime now = RTC_Millis.now();
      //byte hours = (now.hour() + hoursDiff)%24;
      lc.setNumber((value+diff) % maxValue);
        
      unsigned long pressStart=millis();
      while( touchSensor.getButtonsPressed()==buttonsPressed && (millis()-pressStart)<300 )
      {
        lc.flashStatusLeds();
        delay(50);//limit repeat speed. todo: better handle press and hold, to have faster movement through numbers
      }
    }
    else
    {
      delay(50);
    }
  }
  
  
}


void tempMode()
{
  //show temperature
  int8_t temperature = round(getTemperature()-5);
  lc.setNumber(temperature);
  lc.setLed(1, 9, ON);// reuse 'o' for degrees symbol
  unsigned long startTime=millis();
  // wait for all buttons to be released
  while( touchSensor.getButtonsPressed()!=0 )
  {
    lc.flashStatusLeds();
    delay(50);
  }
  // hide temperature after timeout, or a button press wait for input
  while( touchSensor.getButtonsPressed()==0 && (millis()-startTime)<2500 )
  {
    delay(50);//limit repeat speed. todo: better handle press and hold, to have faster movement through numbers
  }
  
  showCurrentTime();
  
  // wait for all buttons to be released
  while( touchSensor.getButtonsPressed()!=0 )
  {
    lc.flashStatusLeds();
    delay(50);
  }  

}


void demoMode()
{
  /* Demo mode */
  lc.flashStatusLeds();
  unsigned long pressStart=millis();
  boolean released = false;
  DateTime now = RTC_Millis.now();
  byte hours = (now.hour()-9)%12;
  byte minutes = now.minute();
  
  for( int i=0; i<60*12; i++ )
  {
    minutes++;
    if( minutes>=60 )
    {
      minutes -= 60;
      hours = (hours+1)%12;
    }
    showTime(hours, minutes);
    
    if( hours==1 && minutes==37 )
    {
      lc.setTicker("http://hackaday.com", 19, false);
      //show ticker till finished, or a button was pressed
      while( !lc.isTickerDone() )
        delay(50);
      lc.stopTicker();
      lc.setLed(3,5,ON);
      delay(500);
      lc.clearDisplay();
      lc.setLed(7,7,ON);
      delay(500);
      lc.clearDisplay();
      lc.setLed(4,1,ON);
      delay(500);
      lc.clearDisplay();
      lc.setLed(2,0,ON);
      delay(500);
      lc.clearDisplay();
      lc.setLed(3,6,ON);
      delay(500);
      lc.clearDisplay();
      lc.setLed(5,0,ON);
      delay(500);
      lc.clearDisplay();
      lc.setLed(7,6,ON);
      delay(500);
      lc.clearDisplay();
      lc.setLed(8,4,ON);
      lc.setLed(8,5,ON);
      delay(500);
      showTime(13,37);
      delay(1000);
    }  
    

    unsigned long startTime=millis();
    int duration = 125 + (15-((15*i)/(60*12)))*25;
    while( (millis()-startTime)<duration )
    {
      if( touchSensor.getButtonsPressed()==0 )
      {
        released = true;
      }
      else 
      {
        lc.flashStatusLeds();
        if( released )
        {
          // re-press detected, so end demo mode
          showCurrentTime();
          // wait till all released
          while( touchSensor.getButtonsPressed()!=0 )
          {
            lc.flashStatusLeds();
            delay(50);
          }
          return;
        }
      }
      delay(50);
    }
    
    
  }//end for
  /* End Demo mode */  
  
}


DateTime lastTimeDisplayed;
void showCurrentTime(boolean conditional)
{
  DateTime now = RTC_Millis.now();
  if( !conditional || lastTimeDisplayed.hour()!=now.hour() || lastTimeDisplayed.minute()!=now.minute() )
    showTime(now.hour()-9, now.minute());
  lastTimeDisplayed = now;
}
void showCurrentTime()
{
  DateTime now = RTC_Millis.now();
  showTime(now.hour()-9, now.minute());
  lastTimeDisplayed = now;
}


void showTime(byte hours, byte minutes)
{
  byte words[6];
  for( int i=0; i<6; i++)
    words[i]=0xFF;
  byte hr = hours%12;
  byte min = round(1.0*minutes/5)*5;// 0..60
  int8_t mindiff = minutes-min;
  if( min==60 )
  {
    min = 0;
    hr  = (hr+1)%12;
  }
  
/*
  Serial.print(hours);
  Serial.print(':');
  Serial.print(minutes);
  Serial.print(' ');
  Serial.print(min);
  Serial.print(" + ");
  Serial.print(mindiff);
  Serial.println();
*/

  byte i=0;
  words[i++] = WORDINDEX_IT;
  words[i++] = WORDINDEX_IS;

  if( min>=20 )
  {
    hr = (hr+1)%12; // 6:20 => tien-voor-half-ZEVEN
  }
  
  if( hr==5 && min==0 )
    words[i++] = WORDINDEX_MINUTE_5;
  else
    words[i++] = hr - WORDINDEX_HOUR_12;

  switch(min)
  {
    case 0:
      words[i++] = WORDINDEX_OCLOCK;
      break;
    case 5:
      words[i++] = WORDINDEX_MINUTE_5;
      words[i++] = WORDINDEX_AFTER;
      break;
    case 10:
      words[i++] = WORDINDEX_MINUTE_10;
      words[i++] = WORDINDEX_AFTER;
      break;
    case 15:
      words[i++] = WORDINDEX_MINUTE_15;
      words[i++] = WORDINDEX_AFTER;
      break;
    case 20:
      words[i++] = WORDINDEX_MINUTE_10;
      words[i++] = WORDINDEX_BEFORE;
      words[i++] = WORDINDEX_HALF;      
      break;
    case 25:
      words[i++] = WORDINDEX_MINUTE_5;
      words[i++] = WORDINDEX_BEFORE;
      words[i++] = WORDINDEX_HALF;      
      break;
    case 30:
      words[i++] = WORDINDEX_HALF;      
      break;
    case 35:
      words[i++] = WORDINDEX_MINUTE_5;
      words[i++] = WORDINDEX_AFTER;
      words[i++] = WORDINDEX_HALF;      
      break;
    case 40:
      words[i++] = WORDINDEX_MINUTE_10;
      words[i++] = WORDINDEX_AFTER;
      words[i++] = WORDINDEX_HALF;      
      break;
    case 45:
      words[i++] = WORDINDEX_MINUTE_15;
      words[i++] = WORDINDEX_BEFORE;
      break;
    case 50:
      words[i++] = WORDINDEX_MINUTE_10;
      words[i++] = WORDINDEX_BEFORE;
      break;
    case 55:
      words[i++] = WORDINDEX_MINUTE_5;
      words[i++] = WORDINDEX_BEFORE;
      break;
  }

  /*  
  for(int i=0; i<6; i++ )
  {
    Serial.print(words[i]);
    Serial.print("\t");
  }
  Serial.println(sizeof(words));
  */
    
  lc.setWords(words, i);

  if( mindiff==-2 )
    lc.setLed(9, 0, true);
  if( mindiff<0 )
    lc.setLed(9, 1, true);
  if( mindiff>0 )
    lc.setLed(9, 2, true);
  if( mindiff==2 )
    lc.setLed(9, 3, true);

}

void fadeIntensity(byte from, byte to, int time)
{
  // e.g. 0 to 15, will immediately start with 1, and then add a delay between 1..2, 2..3, ... , 14..15 (14 delays/steps). After 15 it will immediately return.
  if( from==to )
    return;
  int8_t dir = (to>from) ? +1 : -1;
  byte steps = abs(from-to);
  byte step = dir>0 ? 0 : steps-1;
  int steptime = time/steps;
  byte intensity = from+dir;//start with first step
  const float speedup = 0.5;
  while( intensity!=to )
  {
    step = (intensity-min(to,from))-1;
    lc.setIntensity(intensity);
    float delayMultiplier = 1.0+speedup - step*((2*speedup)/(steps-2));//todo: delay more toward zero, so reverse the effect for fade up / to 15
    /*
    Serial.print(step);
    Serial.print("\t");
    Serial.print(intensity);
    Serial.print("\t");
    Serial.print(delayMultiplier);
    Serial.print("\t");
    Serial.println(delayMultiplier*steptime);
    */
    delay(delayMultiplier*steptime);
    intensity += dir;
  }
  lc.setIntensity(to);
  
}

double getTemperature(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}
