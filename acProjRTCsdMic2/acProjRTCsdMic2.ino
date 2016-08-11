/*
 * My arduino and A/c project
 * uses spi, i2c and serial debug messages
*/
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <SPI.h>

//definitions
#define HOUR_23 23 //11pm
#define HOUR_8 8  //8 am


//function declarations
bool mic_sample();

//variables
static bool prevMicRead = 0; //init at off-will only start recording when ac goes on-has local scope
static uint32_t periodStartTime; //time at the beginning of period we are recording for on time percentage
static uint32_t startTime; //time when the system turned on from being off
static uint32_t onTime = 0; // accumulator of time the system is on
static uint8_t previousHour; //keeps track of what hour it was the last time in the loop

//for rtc
RTC_DS1307 rtc;

//for SD card
File myFile, myFile2;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};



void setup () {

//General init
  Serial.begin(57600);

//RTC init
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

//  SD card init

  Serial.print("Initializing SD card...");
  // CS pin 10
   pinMode(10, OUTPUT);
 
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

 //open file onOff.txt
  myFile = SD.open("onOff.txt", FILE_WRITE);

 if (!myFile) { 
    // if the file didn't open, print an error:
    Serial.println("error opening onOff.txt");
  }

//open second file
  myFile2 = SD.open("prcnt.txt", FILE_WRITE);

 if (!myFile2) { 
    // if the file didn't open, print an error:
    Serial.println("error opening prcnt.txt");
 }

 
  Serial.println("Started recording from here-");
  myFile.println("Started recording from here-");
  myFile.flush();

  myFile2.println("Started recording from here-");
  myFile2.flush();

  DateTime now = rtc.now(); //get time
  periodStartTime = now.unixtime(); //this is the start of the recording period
  Serial.print("periodStartTime ");
  Serial.println(periodStartTime);
  previousHour = now.hour(); //init this variable

  delay(100); //so microphone can get settled

}

void loop () {

    DateTime now = rtc.now(); //get time
    bool micReading = mic_sample(); //read mic

//    only enters if status has changed
//    only records a/c is on or off if status has changed
    if (micReading != prevMicRead)
    { 

  //    first print to screen
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(" ");
    
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
  
      Serial.print(" (");
      Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
      Serial.print(") ");
      Serial.print(micReading);
      Serial.println();
      Serial.println();
  
  //    then print to sd card
      myFile.print(now.year(), DEC);
      myFile.print('/');
      myFile.print(now.month(), DEC);
      myFile.print('/');
      myFile.print(now.day(), DEC);
      myFile.print(" ");
    
      myFile.print(now.hour(), DEC);
      myFile.print(':');
      myFile.print(now.minute(), DEC);
      myFile.print(':');
      myFile.print(now.second(), DEC);
  
      myFile.print(" (");
      myFile.print(daysOfTheWeek[now.dayOfTheWeek()]);
      myFile.print(") ");
      myFile.print(micReading);
      myFile.println();
  
  //    save it to the sd card
      myFile.flush();
      prevMicRead = micReading;

      //now the prevMicRead is the current mic reading either on or off
      //so we can see if it is on or off
      //if on we want to take a time stamp
      //if off - we want to record how long it was on for
      //the first time it enters this loop it will be on
      if (prevMicRead)
      {
        startTime = now.unixtime(); //record current time
        Serial.print("start time ");
        Serial.println(startTime, DEC);
      }
      else if (!prevMicRead)
      {
        onTime += (now.unixtime() - startTime); //accumulate time on
        //the next time it turns on startTime will get initialized;
        Serial.print("accumulating on time ");
        Serial.println(onTime, DEC);

      }
      
      }
      
     //only enters if it just turned 11pm 
    //or it just turned 8 am
    //print to file that records percentage
    //&& and logical has precedence over || or logical
    if  ((now.hour() == HOUR_23 || now.hour() == HOUR_8) && previousHour != now.hour() ) 

//    if (previousHour != now.minute() )

    {
      //if mic is currently on we want to record that time too
      if (micReading)
      {
        onTime += (now.unixtime() - startTime); //accumulate time on
        Serial.print(F("accumulating on time "));
        Serial.println(onTime, DEC);
        //restart the start time
        startTime = now.unixtime();
        Serial.println(F("mic was on "));
      }

      //calculate the percentage of on time of this period
      float percentage = float(onTime) / float(now.unixtime() - periodStartTime) * 100;
      //print date
      myFile2.print(now.year(), DEC);
      myFile2.print('/');
      myFile2.print(now.month(), DEC);
      myFile2.print('/');
      myFile2.print(now.day(), DEC);
      myFile2.print(" ");
      //print time
      myFile2.print(now.hour(), DEC);
      myFile2.print(':');
      myFile2.print(now.minute(), DEC);

      //print day
      myFile2.print(" (");
      myFile2.print(daysOfTheWeek[now.dayOfTheWeek()]);
      myFile2.print(") ");
      //print percent
      myFile2.print(percentage);
      myFile2.println("%");
      //debug stuff
      myFile2.print(F("onTime: "));
      myFile2.print(onTime);
      myFile2.print(F(" now: "));
      myFile2.print(now.unixtime());
      myFile2.print(F(" periodStart: "));
      myFile2.println(periodStartTime);

      //i forgot this!!
      myFile2.flush();

      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(" ");
      //print time
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);

      //print day
      Serial.print(" (");
      Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
      Serial.print(") ");
      //print percent
      Serial.print(percentage);
      Serial.println("%");

            //debug stuff
      Serial.print(F("onTime: "));
      Serial.print(onTime);
      Serial.print(F(" now: "));
      Serial.print(now.unixtime());
      Serial.print(F(" periodStart: "));
      Serial.println(periodStartTime);


      //reset values
      periodStartTime = now.unixtime();
      onTime = 0;
    Serial.print(F("periodStartTime"));
    Serial.println(periodStartTime);
    Serial.print(F("onTime "));
    Serial.println(onTime);

//      if (micReading) 
//      {
//        startTime = now.unixtime();
//        Serial.print(F("mic was on so startTime is "));
//    Serial.println(startTime);
//
//      }

    
    }

      previousHour = now.hour(); //update this variable
//    previousHour = now.minute();
//    Serial.print(F("previousHour "));
//    Serial.println(previousHour);
    delay(5000);
   
}

bool mic_sample() 
{
   const int sampleWindow = 500; // Sample window width in mS (50 mS = 20Hz)
   unsigned int sample;
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;
   bool onOff;

   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(0);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
   double volts = (peakToPeak * 3.3) / 1024;  // convert to volts

   Serial.print(volts);
   Serial.println(F(" Volts"));
   
   if (volts > .15) onOff = 1;
   else onOff = 0;
   

   return onOff;
}
