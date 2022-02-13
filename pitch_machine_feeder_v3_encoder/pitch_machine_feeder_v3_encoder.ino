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
 * Date: 2/12/2022
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
void adjustRotation();
void timingAnimation();
void buttonPress();
void remotePress();


//VARIABLES

//button data
#define BUTTON_PIN 2
#define REMOTE_PIN 3

//potentiometer data
#define POT_PIN 0
long pot_val = 0;

//light variables

//digital pin on the Arduino that is connected to the NeoPixels
#define LED_PIN 15

//how many NeoPixels are attached to the Arduino
#define NUM_PIXELS 8

const float MS_BETWEEN_LED = 5000/NUM_PIXELS;
float ms_before_led = 0;

//brightness level of LED's (between 0 and 255)
#define BRIGHT 20

Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN);


//create stepper motor with 200 steps per revolution on port 1
//our stepper motor is designed for 200 steps per revoultion
//M1 and M2 go to port 1, M3 and M4 go to port 2
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor* my_motor = AFMS.getStepper(200, 1);

//number of baseballs the device holds
#define MAX_PITCHES 10

//number of pitches thrown
int pitch_count = 0;

//time in between pitches
long delay_time = 0;

//wait 5 seconds before starting pitching
#define STARTING_DELAY 5*1000    //performance version is 60 seconds

//currently throwing pitches
//must be volatile so that it can be altered inside interrupt
volatile bool active = false;

//number of degrees the cam is over-rotated
int deg_over_rotated = 0;


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
  my_motor->setSpeed(20);

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
  if (active == true && pitch_count!=MAX_PITCHES){
    
    //read potentiometer value
    pot_val = analogRead(POT_PIN);
    //convert analog (0-1023) to digital (0-255)
    pot_val = map(pot_val, 0, 1023, 0, 255);
  
    //calculate the delay time to be between 5 and 10 seconds (depending on where the potentiometer is set)
    delay_time = pot_val*(1000)/51 + 5000;
  
    //LED animation starts with 5 seconds remaining until pitch
    ms_before_led = delay_time - 5000;

    //complete the pitch with lights
    pitching();
  }

  //clear lights and reset count if not pitching
  if(false == active){
    pitch_count = 0;
    pixels.fill(pixels.Color(BRIGHT/4, BRIGHT/4, BRIGHT/4), 0);
    pixels.show();
  }

}


//get reading from encoder and move stepper motor backwards to align on axis
void calibrate(){
  int deg;
  //srt get encoder reading
  //do math to figure out how many degrees to rotate

  //move backwards to align on an axis (forward variable = backward rotation) and lock rotation
  my_motor->step(deg, FORWARD, DOUBLE);
}


//executes one pitch with lights
//if at any point 'active' changes to false, immediately returns
void pitching(){
  
  //delay before first pitch and set LEDs to red
  if(pitch_count == 0){
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
    delay(ms_before_led/50);
  }

  //adjust amount to be rotated based on position of cam
  adjustRotation();

  //run light animation for the 5 seconds before pitching
  timingAnimation();

  //check again to make sure we haven't pressed the button
  if(false == active){
    return;
  }

  //flash green right as we are pitching
  pixels.fill(pixels.Color(0, BRIGHT, 0), 0);
  pixels.show();
  
  //turn stepper motor 90 degrees, accounting for over-rotated cam
  my_motor->step(50-deg_over_rotated, BACKWARD, DOUBLE);
  pitch_count++;

  //if done, change to inactive
  if(pitch_count == MAX_PITCHES){
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


void adjustRotation(){
  //srt read value from encoder
  //do math to figure out how many degrees it is off
  //set deg_over_rotated (positive = over-rotated, negative = under-rotated)
}


//flips the active status if the button is pressed
void buttonPress(){
  if(true == button_pressed){
    return;
  }
  
  //account for debounce by requiring 500 milliseconds to pass before considering a new press
  if(millis() - button_time > 500){
    button_count++;
    button_time = millis();
    button_pressed = true;
    
    //flip the active variable
    active = !active;   
  }
}


//if the remote button is pressed
void remotePress(){
  if(true == remote_pressed){
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
