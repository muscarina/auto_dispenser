// First we include the libraries
#include <OneWire.h> 
#include <DallasTemperature.h>
/********************************************************************/
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 2

//Internal LED of arduino boards
int led = 13;

//The temperature to light up the LED at
float set_temp = 30.0;
int led_status;
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 
void setup(void) 
{ 
pinMode(led, OUTPUT);

 // start serial port 
 Serial.begin(9600); 
 Serial.println("Dallas Temperature IC Control Library Demo"); 
 // Start up the library 
 sensors.begin(); 
} 
void loop(void) 
{ 
 // call sensors.requestTemperatures() to issue a global temperature 
 // request to all devices on the bus 
/********************************************************************/
 sensors.requestTemperatures(); // Send the command to get temperature readings 

 float current_temp = sensors.getTempCByIndex(0);
/********************************************************************/
 Serial.print("Temperature is: "); 
 Serial.println(current_temp); // Why "byIndex"?  
   // You can have more than one DS18B20 probes on the same bus.  
   // 0 refers to the first IC on the wire 
   delay(1000); 


   if (abs(current_temp - set_temp) < .3){ //Check if current_temp - set_temp is smaller than 0.3C
    
      if(led_status == 0) { //LED is OFF
         Serial.println("Temp >= 30C, LED ON.");
         led_status = 1;
         digitalWrite(led, HIGH); // turn the LED on (HIGH is the voltage level)
      }
         
        

   }
   else{
         if(led_status == 1) { //LED is ON
            Serial.println("Temp >= 30C, LED OFF.");
            led_status = 0;
            digitalWrite(led, LOW); // turn the LED off (LOW is the voltage level)
           }
           

   }



} 
