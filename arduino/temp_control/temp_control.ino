//Hardware:
//Temp probe: DS18B20
//Microcontroller: Arduino Nano
//Screen: 128x32 OLED
/********************************************************************/
// First we include the libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Buzzer.h>

#include <Adafruit_SSD1306.h>
/********************************************************************/
//PINS
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

//*Pin assignment
#define relay 13 //Relay module pin (or internal  LED pin 13)
#define Button 4 //Relay module pin 3

Buzzer buzzer(3); //Buzzer pin


//*variables
unsigned long previousMillis1 = 0; //Earlier timestamp, relay 1
unsigned long previousMillis2 = 0; //Earlier timestamp, relay 2
unsigned long previousMillis3 = 0; //Earlier timestamp, relay 2 blocking script.
unsigned long previousMillis4 = 0; //Earlier timestamp,Button debounce


//*Relay variables
int relayState = 0; //Current state of relay
int relayCount = 0; //Integer variable to keep track of how many times the relay has been opened.
const int relayCountLimit = 6; //Limits how many times the oxygen relay can be opened  before blocking the  relay.

int blockRelay = 0; //?Variable for blocking the nitrous relay when sensor levels are critical
byte openRelayFlag = HIGH; //Variable that allows the nitrous relay to be opened.

/********************************************************************/

//*Relay delays and intervals
const int relayDelay = 10000; //How many ms the relay stays off before being being available again.
const int relayInterval = 2000; //Amount of time in ms the relay stays open

const int relayBlockDelay = 500; //How long the delay is between different blocking periods. Keep this low so it's blocked when it has been told to be.
const int relayBlockTimer = 12000; //How long in ms the  relay stays blocked after being opened too often
/********************************************************************/

//*Buttons
int ButtonState = HIGH; // variable for reading the pushButton status
const int ButtonDebounceTime = 50; //debounce Button time in milliseconds.
/********************************************************************/

//!! DEBUG FLAG
int DEBUG = 1;

float tempThresholdLimit = 28.0; //temp limit in celsius

float tempCritThresholdLimit = 31.0; //critical temp limit in celsius, will sound an alarm if reached and block the relay from opening

int tempThresholdState; // 0 for OK, 1 for threshold, 2 for critical threshold

float currentTemperature;

/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
/*************************************************/

//*initiate screen
#define OLED_ADDR 0x3C // OLED display TWI address
Adafruit_SSD1306 display(-1);

////////////!! SETUP
void setup() {

    //Start temp sensor probe library
    sensors.begin();

    //? Pin modes
    pinMode(relay, OUTPUT);
    pinMode(Button, INPUT_PULLUP);

    //? Default states
    digitalWrite(relay, LOW);

    //?Attach an interrupt to the ISR vector
    //attachInterrupt(digitalPinToInterrupt(Button), pin_ISR, CHANGE);

    Serial.begin(9600); //Start serial

    // enable debug to get addition information
    // co2.setDebug(true);

    //Screen startup
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR); // changed this to 0x3C to make it work
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.display();
}
////////////!! SETUP

//!FUNCTIONS
void InitiateDisplay(int iTextSize) { //Runs commands that prepares display for data
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(iTextSize);
}

void displayValuesScreen() { //Main display function for sensor values
    InitiateDisplay(2); //Initiates display with defined text size.
    display.print("CO2:");
    display.print("O2:");
    display.print(currentTemperature);
    display.print("%");
    display.display();
}

void displayValuesSerial() { //Main display function for sensor values
    Serial.print("Temp: ");
    Serial.print(currentTemperature);
    Serial.println(" C");
}
//Sound a buzzer
void alarm_buzzer() { //Buzzer alarm function
  buzzer.begin(10);
  buzzer.sound(NOTE_E7, 80);
  buzzer.sound(NOTE_E7, 80);
  buzzer.sound(0, 80);
  buzzer.sound(NOTE_E7, 80);
  buzzer.sound(0, 80);
  buzzer.sound(NOTE_C7, 80);
  buzzer.sound(NOTE_E7, 80);
  buzzer.sound(0, 80);
  buzzer.sound(NOTE_G7, 80);
  buzzer.sound(0, 240);
  buzzer.sound(NOTE_G6, 80);
  buzzer.end(10);
  }
//Call with 1 for O2, 2 for CO2 and 3 for both.
void Alarm(){
        display.print("CO2:");
        display.print("O2:");
        display.print(currentTemperature);
        display.println("%");
        display.display();

        Serial.println("Warning! Temp is over the set critical limit.");
        
        alarm_buzzer();
}

void writeRelayStateToSerial(int relayState) {
    if (relayState == 1) { //Open
        Serial.println("Opening relay ");
    } else { //Closed
        Serial.println("Closing relay");
    }
}

/*void pin_ISR() {
    noInterrupts(); //Make sure this call to ISR is not blocked by a new one.
    if(openRelayFlag == HIGH && blockRelay == 0 && relayState_2 == 0){
        openRelayFlag = LOW;
    }
    interrupts();  //Enable interrupts again.
}*/


void displaySensorValues(){//Reads sensor values and them displays them over serial and OLED
      displayValuesScreen(); //Display sensor values on screen.
      displayValuesSerial(); //Display sensor values on serial port.
   
    if(tempThresholdState == 2){ //Critical threshold
      Alarm();
    }

}
//Checks if  Button has been pressed
void checkButtonState(){
    if (digitalRead(Button) == LOW && openRelayFlag != HIGH && tempThresholdState == 1){//Read  Button state + check that relay is not open already + temp within thresholds
        openRelayFlag = HIGH;
    }
}

int checkThresholds(){//Checks sensor values against threshold variables.
        if(abs(currentTemperature - tempThresholdLimit)  < .1 || currentTemperature < tempThresholdLimit){ //value within tempThresholdLimit by 0.1C or smaller than tempThresholdLimit 
            return(1);
             Serial.println("Temp value within configured tolerance.");
        }
        else if(abs(currentTemperature - tempCritThresholdLimit)  < .1 || currentTemperature > tempCritThresholdLimit){ //value within tempCritThresholdLimit by 0.1C or larger than.
            return(2);
             Serial.println("Temp value over critical threshold");
        }
        else
            return(0);//Value is under threshold
}

void checkRelayBlock(unsigned long currentMillis){ //Blocks relay when needed and handles the relayCount unlocking.
    if(blockRelay == 0){
            if(relayCount >= relayCountLimit) //If relay has fired too often
                blockRelay = 1; //Block  relay

      else if(tempThresholdState == 2) //If critical threshold has been reached
                blockRelay = 1; //Block  relay

      if (relayCount >= relayCountLimit && (tempThresholdState != 2)) { //If nitrous relay has been opened too often lately and there are no threshold alarms.
          if (currentMillis - previousMillis3 >= relayBlockDelay) { //Relay is open and has been opened for the set duration
                blockRelay = 1;
                previousMillis3 = currentMillis; //Save the time we blocked the relay.
                relayCount = 0; //reset count
            }
        }
    }
  else if (blockRelay == 1){ //Code for currently active blocks
        if (currentMillis - previousMillis3 >= relayBlockTimer && (tempThresholdState != 2))//Relay has been blocked for the set duration and thereare no threshold alarms currently active
                blockRelay = 0;

        if (tempThresholdState == 0) //See if  relay block initated from sensor alarms can be removed
                blockRelay = 0;
    }

    if (relayCount > 0 && tempThresholdState != 2){ //Reset relay counter after a successful raising of O2 concentration above threshold limit.
            relayCount = 0;}

    if(openRelayFlag == LOW && blockRelay == 1){ //If relay has a block enabled, set openRelayFlag to HIGH as it wont be opening now anyway.
            openRelayFlag = HIGH;}
}

void openRelay(unsigned long currentMillis){
    //Check sensor values against pre-defined threshold values or if relay is currently opened.
        if (tempThresholdState == 1 || relayState == 1) 
           { 
            if (currentMillis - previousMillis1 >= relayDelay && relayState == 0) { //Don't open relay too often
                previousMillis1 = currentMillis; //Save the  time the relay was opened
                relayCount++;
                relayState = 1;
                digitalWrite(relay, HIGH); //Open relay
                Serial.println("Relay is open");

                writeRelayStateToSerial(relayState);
            }
            else if (currentMillis - previousMillis1 >= relayInterval && relayState == 1) { //Relay is open and has been opened for the set duration
                relayState = 0;
                digitalWrite(relay, LOW); //Close relay
                 writeRelayStateToSerial(relayState);
            }

    }

}

float readTemperatures(){ //Reads temps from probes
  
  //Step 1: issue a global temperature request to all devices on the OneWire bus
  sensors.requestTemperatures(); // Send the command to get temperature readings
  
  //Step 2: get temp from prob
  return sensors.getTempCByIndex(0);

  // Why "getTempCByIndex"?
  // You can have more than one DS18B20 probes on the same bus.
  // 0 refers to the first IC on the wire
 
}

//!! MAIN
void loop() {
    //InitiateDisplay(1); //Initiates display with defined text size.

    //get current probe temperature
    currentTemperature = readTemperatures(); 

    //Check if temperature falls within set thresholds
    tempThresholdState = checkThresholds(); //Returns either 0,1,2 (1 = value >= set threshold, 2 = value > critical threshold)

    Serial.print("tempThresholdState is: " );
    Serial.println(tempThresholdState );

    
    //Read and display the sensor values on serial port and on screen.
    displaySensorValues(); 

    checkRelayBlock(millis()); //Check if  relay should be blocked from opening.

    openRelay(millis()); //Check if relays need opening
    
    checkButtonState(); //check if button for manual trigger is pushed

        }
