/*
 ********************************
 Latest Date: 3 Ogos 2018
 Latest Time: 13:00:00 PM
 PLATFORM: ARDUINO MEGA
 ********************************
 
 The circuit of I2C LCD:
 * SCL pin to D21(SCL)
 * SDA pin to D20(SDA)
 * VCC pin to 5V
 * Gnd pin to GND
  
 The circuit of DHT22
 * Signal pin to A2
 * VCC pin to 3,3V
 * GND pin to GND
 The circuit of DS18B20
 * Signal pin to A3
 * VCC pin to 3.3V
 * Gnd pin to GND
 The circuit of RTC DS1307 module
 * SCL pin to D21(SCL)
 * SDA pin to D20(SDA)
 * VCC pin to 5V
 * Gnd pin to GND
 The circuit of SDCard module
 * SCK pin to D52
 * MISO pin to D50
 * MOSI pin to D51
 * CS pin to D53
 * VCC pin to 5V
 * Gnd pin to GND 
 
*/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>

// -----------------Address,Col,Row
LiquidCrystal_I2C lcd(0x27 ,16 ,2);


OneWire oneWire(A3);
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature. 
DeviceAddress Sensor1 = { 0x28, 0xFF, 0xCD, 0x97, 0xA1, 0x17, 0x05, 0x01 };
DeviceAddress Sensor2 = { 0x28, 0xFF, 0xA6, 0x9A, 0xA2, 0x17, 0x05, 0x19 };
int amountOfSensors = 2;
int counterCheckBus = 0;
float It = 0.0;
float Ot = 0.0;


RTC_DS1307 RTC;
int systick = 0;
int prev_systick = 0;
int T1_tick = 0;
int T2_tick = 0;
int T3_tick = 0;

#define chipSelect 53  //SDcard
String dataString = "";

void setup() {
  Serial.begin(9600);
  Serial.println(); 
  Serial.println("====================================");
  Serial.println("Temperature and Humidity Data Logger"); 
  Serial.println("------------------------------------");
  //---[1] LCD
  Serial.print("[1] Initializing LCD..."); 
  lcd.init();           
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print("Initializing...");
  Serial.println("done"); 


  //---[2/3] DS18B20 IN OUT
  Serial.print("[3] Initializing DS18B20..."); 
  sensors.begin();
  sensors.setResolution(Sensor1, 10);
  sensors.setResolution(Sensor2, 10);
  getSensorsOnBus();
  Serial.println("done"); 


  //---[4] RTC
  Serial.print("[4] Initializing RTC DS1307..."); 
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
   RTC.adjust(DateTime(__DATE__, __TIME__));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  Serial.println("done"); 

  //---[5]SDCard
  Serial.print("[5] Initializing SDCard..."); 
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    lcd.setCursor(0, 1); 
    lcd.print("SDCard failed!");
    while (1);
  }
  Serial.println("done"); 

  Serial.println("---------------->SYSTEM READY"); 
  delay(1000);  //delay for stabilize data of measurement
  lcd.clear();
}

void loop() {

  //---Read DS18B20
    counterCheckBus++;
  delay(100);
  Serial.println();

  if (counterCheckBus == 360)
  {
    getSensorsOnBus();
    counterCheckBus = 0;
  }
  
  //---Read RTC
   DateTime now = RTC.now();
  int systick = now.second();
  
  //---System Update Rate (every second)
  if(prev_systick != systick){
    //---[T1] Read sensor Outdoor data, Indoor data and update onto LCD
    if(T1_tick == 1){
      sensors.requestTemperatures();
      It = sensors.getTempC(Sensor2);
      Ot = sensors.getTempC(Sensor1);
      lcd.setCursor(0, 1); 
      lcd.print("0:"); lcd.print(Ot,1); lcd.print("C "); lcd.print("I:"); lcd.print(It,1); lcd.print("C");
      T1_tick = 0;
    }
    
     //---[T2] Read Time and update onto LCD 
    //if(T2_tick == 0){
      lcd.setCursor(0, 0); 
      lcd.print(now.day(), DEC);
      lcd.print('/'); 
      lcd.print(now.month(), DEC);
      lcd.print('/');
      lcd.print(now.year(), DEC);
      lcd.print(' ');
      if(now.hour()<10) lcd.print('0'); 
      lcd.print(now.hour(),DEC);
      lcd.print(':');
      if(now.minute()<10) lcd.print('0'); 
      lcd.print(now.minute(),DEC);
      
    //  T2_tick = 0;
    //}
  
    //---[T3] Log data into SDCard
    if(T3_tick == 9){
    //----------- Date
      if(now.day()<10) dataString += '0'; dataString += String(now.day(), DEC);
      dataString += "-"; 
      if(now.month()<10) dataString += '0';dataString += String(now.month(), DEC);
      dataString += "-"; 
      dataString += String(now.year(), DEC);
      dataString += ","; 
      //------------Time
      if(now.hour()<10) dataString += '0';
      dataString += String(now.hour(), DEC);
      dataString += ":"; 
      if(now.minute()<10) dataString += '0';
      dataString += String(now.minute(), DEC);
      dataString += ":"; 
      if(now.second()<10) dataString += '0';
      dataString += String(now.second(), DEC);
      dataString += ","; 
      //------------Indoor Temperature
      dataString += String(It,1);
      dataString += ",";   
      //------------Outdoor Temperature
      dataString += String(Ot,1);
      dataString += ",";   
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
      if (dataFile) {  // if the file is available, write to it:
        dataFile.println(dataString);
        dataFile.close();
        // print to the serial port too:
        Serial.println(dataString);
        dataString = "";
      } else {  // if the file isn't open, pop up an error:
        Serial.println("...error opening datalog.txt");
      }
      T3_tick = 0;
    }else{
      T1_tick++;
    //  T2_tick++;
      T3_tick++;
    }
  } 
  prev_systick = systick;
}

void getSensorsOnBus() {
  Serial.print("Number of sensors found on bus: ");
  Serial.println(sensors.getDeviceCount());
}
