/***********************************************************
 * Authors: Sadie Thomas and Alex Prestwich
 * Description: Rotates a motor to release baseballs into 
 *    pitching machine based on button/rf input and 
 *    coordinates LED indicator lights
 * Inputs: potentiometer, button, rf signals
 * Outputs: Serial prints, LED sequences, motor rotation
 * Components: Arduino Uno, Adafruit MotorShield, NEMA 17 
 *    stepper motor, Adafruit Neopixel LED strip, 
 *    momentary rf reciever/emitter, 10K ohm potentiometer, 
 *    momentary button
 * Date: 1/22/2022
 ***********************************************************/

#include <Wire.h>
#include <Adafruit_MotorShield.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


//FUNCTION PROTOTYPES

void calibrate();
void pitching();
void timingAnimation();
void buttonPress();
void remotePress();


//VARIABLES

//button data
#define BUTTON_PIN 2
#define REMOTE_PIN 3

//if the machine is being calibrated
//volatile so it can be changed inside interrupt
volatile bool first_press = true;

//potentiometer data
#define POT_PIN 0
long potVal = 0;

//light variables

//digital pin on the Arduino that is connected to the NeoPixels
#define LED_PIN 15

//how many NeoPixels are attached to the Arduino
#define NUM_PIXELS 8

const float MS_BETWEEN_LED = 5000/NUM_PIXELS;
float msBeforeLed = 0;

//brightness level of LED's (between 0 and 255)
#define BRIGHT 20

Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN);


//create stepper motor with 200 steps per revolution on port 1
//M1 and M2 go to port 1, M3 and M4 go to port 2
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1);

//number of baseballs the device holds
#define MAX_PITCHES 10

//number of pitches thrown
int pitchCount = 0;

//time in between pitches
long delayTime = 0;

//wait 5 seconds before starting pitching
#define STARTING_DELAY 5*1000    //performance version is 60 seconds

//currently throwing pitches
//must be volatile so that it can be altered inside interrupt
volatile bool active = false;


//interrupt variables (volatile so that we can edit it within an interrupt)
volatile bool remote_pressed;
volatile bool button_pressed;

//number of times remote/button have been pressed
volatile unsigned int remote_count;
volatile unsigned int button_count;

//time since remote/button was last pressed
volatile unsigned long remote_time = 0;
volatile unsigned long button_time = 0;


void setup() {
  //setup serial monitor at 9600 bps
  Serial.begin(9600);
  Serial.println("START AUTOMATED BASEBALL FEEDER");

  //setup input button
  //high==on==pressed low==off==not pressed
  pinMode(BUTTON_PIN, INPUT);
  pinMode(REMOTE_PIN, INPUT);
  
  //setup interrupt to toggle 'active' variable if the button is pressed
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPress, FALLING); 
  attachInterrupt(digitalPinToInterrupt(REMOTE_PIN), remotePress, RISING); 
  
  //initialize the stepper motor with speed 20RPMS
  AFMS.begin();
  myMotor->setSpeed(20);

  //initilize NeoPixel strip object
  pixels.begin();
  pixels.clear();
  pixels.show();

  //wait for user to calibrate stepper motor position
  calibrate(); 
}

void loop() {

  //disable interrupts while we access volatile memory larger than 8 bits
  noInterrupts();
  if(true == remote_pressed){
    Serial.print("Remote signal (");
    Serial.print(remote_count);
    Serial.println(") detected");
    remote_pressed = false;
  }
  if(true == button_pressed){
    Serial.print("Button signal (");
    Serial.print(button_count);
    Serial.println(") detected");
    button_pressed = false;
  }
  interrupts();

  //turn stepper motor maxPitches times (or until the off button is pressed)
  if (active == true && pitchCount!=MAX_PITCHES){
    
    //read potentiometer value
    potVal = analogRead(POT_PIN);
    //convert analog (0-1023) to digital (0-255)
    potVal = map(potVal, 0, 1023, 0, 255);
  
    //calculate the delay time to be between 5 and 10 seconds (depending on where the potentiometer is set)
    delayTime = potVal*(1000)/51 + 5000;
  
    //LED animation starts with 5 seconds remaining until pitch
    msBeforeLed = delayTime - 5000;

    //complete the pitch with lights
    pitching();
  }

  //clear lights and reset count if not pitching
  if(false == active){
    pitchCount = 0;
    pixels.fill(pixels.Color(BRIGHT/4, BRIGHT/4, BRIGHT/4), 0);
    pixels.show();
  }

}


//blink lights while waiting for user to calibrate stepper motor position
void calibrate(){
  //blink red lights until user presses the button (indicating calibration done)
  //first_press changes value inside interrupt
  while(true == first_press){
    pixels.fill(pixels.Color(BRIGHT, 0, 0), 0);
    pixels.show();
    delay(100);
    if(false == first_press){
      break;
    }
    pixels.clear();
    pixels.show();
    delay(100);
  }

  //turn lights off
  pixels.clear();
  pixels.show();

  //move one step backwards (forward variable = backward rotation) to lock rotation
  myMotor->step(1, FORWARD, DOUBLE);
}


//executes one pitch with lights
//if at any point 'active' changes to false, immediately returns
void pitching(){
  
  //delay before first pitch and set LEDs to red
  if(pitchCount == 0){
    pixels.fill(pixels.Color(BRIGHT, 0, 0), 0);
    pixels.show();
    for(int i=0; i<100; i++){
      if(false == active){
        return;
      }
      delay(((unsigned long) STARTING_DELAY)/100);
    }
  }

  //delay before the 5-second animation
  for(int i=0; i<50; i++){
    if(false == active){
      return;
    }
    delay(msBeforeLed/50);
  }

  //run light animation for the 5 seconds before pitching
  timingAnimation();

  //check again to make sure we haven't pressed the button
  if(false == active){
    return;
  }

  //flash green right as we are pitching
  pixels.fill(pixels.Color(0, BRIGHT, 0), 0);
  pixels.show();
  
  //turn stepper motor 90 degrees
  myMotor->step(50, BACKWARD, DOUBLE);
  pitchCount++;

  //if done, change to inactive
  if(pitchCount == MAX_PITCHES){
    active = false;
  }

  //refill with red
  pixels.fill(pixels.Color(BRIGHT, 0, 0), 0);
  pixels.show();
}


//5-second animation for timing
//returns if active goes to false
void timingAnimation(){
  for(int i=0; i<NUM_PIXELS; i++){
    if(false == active){
      return;
    }
    pixels.setPixelColor(i, pixels.Color((BRIGHT + BRIGHT/2), BRIGHT, 0));
    pixels.show();
    delay(MS_BETWEEN_LED);
  }
}


//flips the active status if the button is pressed
void buttonPress(){
  if(true == button_pressed){
    return;
  }
  
  //first press indicates calibration is over
  if(true == first_press){
    first_press = false;
    
    //record the time at which the button was pressed
    button_time = millis();
  }
  //following presses indicate to flip the active status variable
  //account for debounce by requiring 500 milliseconds to pass before considering a new press
  else if(millis() - button_time > 500){
    button_count++;
    button_time = millis();
    button_pressed = true;
    
    //flip the active variable
    active = !active;   
  }
}


//if the remote button is pressed
void remotePress(){
  if(true == remote_pressed || true == first_press){
    return;
  }
  
  //account for debounce by requiring 500 milliseconds to pass before considering a new press
  if(millis() - remote_time > 500){
    //possibly change this to remote_count = (remote_count + 1) % 2; so that we only are storing 0 or 1
    remote_count++;

    //the rf receiver triggers the interrupt twice per press, so only act on every other press
    if(remote_count % 2 == 1){
      remote_time = millis();
      remote_pressed = true;

      //flip the active variable
      active = !active;
    }    
  }
}

/*
 * Future possibilities
 *  - add release() after each movement to catch the next baseball? - needs user calibration, works for now (might use encoder later)
 *  - mess around with stepper motor speed (slower may catch a new baseball better) - 20rpms seems better when plugged into wall
 *  - add a power switch

 *  - add capability for the users to set number of pitches? - don't worry about that for now
 *  - improve animation?
 *  - how bright do we want the light to be
 *  - add startup light show
 */
