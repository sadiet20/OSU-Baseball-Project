#include <Wire.h>
#include <Adafruit_MotorShield.h>
//#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

//button data
const int buttonPin = 2;

//time since button was last pressed
int button_pressed = 0;

//potentiometer data
const int potPin = 0;
int potVal = 0;

//create stepper motor with 200 steps per revolution on channel 2 (srt consider changing to 1?)
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1);

//number of baseballs the device holds
const int maxPitches = 10;

//number of pitches thrown
int pitchCount = 0;

//time in between pitches
int delayTime = 0;

//time to wait before starting pitching
const int startingDelay = 0;  //30*1000;

//currently throwing pitches
bool active = false;

//remote variables
const int remotePin = 3;

//light variables

//digital pin on the Arduino that is connected to the NeoPixels
#define LED_PIN 6

//how many NeoPixels are attached to the Arduino
#define NUM_PIXELS 8

const float MS_BETWEEN_LED = 5000/NUM_PIXELS;
float msBeforeLed = 0;

//Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN);


//change consts to DEFINES

void setup() {
  //setup serial monitor at 9600 bps
  Serial.begin(9600);
  Serial.println("START AUTOMATED BASEBALL FEEDER");

  //setup input button
  //high==on==pressed low==off==not pressed
  pinMode(buttonPin, INPUT);
  //pinMode(remotePin, INPUT);

  //setup interrupt to toggle 'active' variable if the button is pressed
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPress, FALLING);
  //attachInterrupt(digitalPinToInterrupt(remotePin), remotePress, RISING);

  //initialize the stepper motor with speed 40RPMS
  AFMS.begin();
  myMotor->setSpeed(40); 

  /*
  //initilize NeoPixel strip object
  pixels.begin();
  pixels.clear();
  pixels.show();
  pixels.fill(pixels.Color(255, 0, 0), 0);    //sets all pixels to red
  */
}

void loop() {
  //read potentiometer value
  //convert analog (0-1023) to digital (0-255)
  potVal = analogRead(potPin);
  potVal = map(potVal, 0, 1023, 0, 255);

  //calculate the delay time to be between 5 and 10 seconds (depending on where the potentiometer is set)
  delayTime = potVal*(1000)/51 + 5000;

  //LED animation starts with 5 seconds remaining until pitch
  msBeforeLed = delayTime - 5000;
  
  //turn stepper motor maxPitches times (or until the off button is pressed)
  if (active == true && pitchCount!=maxPitches)
  {
    delay(msBeforeLed);

    for(int i=0; i<NUM_PIXELS; i++){
      //pixels.setPixelColor(i, pixels.Color(0, 255, 255));
      //pixels.show();
      delay(MS_BETWEEN_LED);
    }

    //pixels.fill(pixels.Color(0, 255, 0), 0);
    //pixels.show();
    
    //turn stepper motor 90 degrees
    myMotor->step(50, BACKWARD, DOUBLE);
    pitchCount++;
    delay(1000);
    
    //pixels.fill(pixels.Color(255, 0, 0), 0);
    //pixels.show();
  }
  else{
    //pixels.clear();
    //pixels.show();
    Serial.println();
    Serial.println("ELSE");
    Serial.println();
  }

  Serial.println();
  Serial.print("potVal: ");
  Serial.println(potVal);
  Serial.print("delay: ");
  Serial.println(delayTime);
  Serial.print("active: ");
  Serial.println(active);
  Serial.print("pitch count: ");
  Serial.println(pitchCount);
  Serial.print("button: ");
  Serial.println(digitalRead(buttonPin));
}


//if the button is pressed, toggle the active variable
//if turning on, reset pitchCount
void buttonPress(){
  //ignore interrupt if the button was pressed within the last two seconds
  if(millis() - button_pressed < 2000){
    return;
  }
  Serial.println("Button has been pressed.");
  button_pressed = millis();
  active = !active;
  if(active == true){
    pitchCount = 0;
    //pixels.show();
    //delay(startingDelay);
  }
}


void remotePress(){
  //make sure we recieved input
  Serial.println("Remote signal detected");
  
  buttonPress();
}


/*
Lights
- red when active
- yellow bar filling to indicate timing
  - all full, servo moves
  - ratio for timing
      - how many LEDs in strip
- flash green right before moving

*/
