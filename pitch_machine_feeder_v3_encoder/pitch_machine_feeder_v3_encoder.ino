/***********************************************************
 * Authors: Sadie Thomas and Alex Prestwich
 * Description: Rotates a motor to release baseballs into 
 *    pitching machine based on button/rf input and 
 *    coordinates LED indicator lights
 * Inputs: potentiometer, button, rf signals
 * Outputs: Serial prints, LED sequences, motor rotation
 * References: Encoder code - Curious Scientist
 * Components: Arduino Uno, Adafruit MotorShield, NEMA 17 
 *    stepper motor, Adafruit Neopixel LED strip, 
 *    momentary rf reciever/emitter, 10K ohm potentiometer, 
 *    momentary button
 * Date: 4/30/2022
 ***********************************************************/

#include <Wire.h>     // for encoder
#include <Adafruit_MotorShield.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


//FUNCTION PROTOTYPES

void calibrate();
void checkMagnetPosition();
void calcDelayTime();
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

//number of degrees to rotate the cam
int turn_distance = 50;


//encoder variables
//first quadrant angle indicating correct cam positioning
//value between 0 and 50 degrees since 200 is a full rotation
#define UPRIGHT 35

//current angle of cam (from 0 to 200)
int cur_angle;
int starting_angle;

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
  //while(!Serial);
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

  //initialize encoder communication
  Wire.begin();             //start i2C  
  Wire.setClock(800000L);   //fast clock

  //calibrate stepper motor to upright position
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

    //calculate the delay time to be between 5 and 10 seconds (depending on where the potentiometer is set)
    calcDelayTime();

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
  int correction;

  //verify encoder setup and get current angle
  checkMagnetPosition();
  getAngle();

  //correct to the previous 90 degree axis (90 degrees == 50 on stepper)
  correction = cur_angle % 50;
  if(correction < 0){         //adjust if negative
    correction += 50;
  }

  //srt consider correction < 5
  if(correction == 0){
    //move backwards to align on an axis (forward variable == backward rotation) and lock rotation
    my_motor->step(1, FORWARD, DOUBLE);
    return;
  }
 
  Serial.print("correction angle (200): ");
  Serial.println(correction);

  //move backwards to align on an axis (forward variable == backward rotation) and lock rotation
  my_motor->step(correction, FORWARD, DOUBLE);

  //srt testing
  Serial.println("New location after calibration: ");
  getAngle();
}


//verify encoder magnet positioned correctly 
void checkMagnetPosition(){
  int first_attempt = 1;
  int magnet_status = 0;
  
  //Status register output: 0 0 MD ML MH 0 0 0  
  //MH: Too strong magnet - 100111 - DEC: 39 
  //ML: Too weak magnet - 10111 - DEC: 23     
  //MD: OK magnet - 110111 - DEC: 55
  
  //while magnet is not positioned at the proper distance (32 == MD set to 1)
  while((magnet_status & 32) != 32){
    //reset status
    magnet_status = 0;

    //ask for status values from MD, ML, and MH
    Wire.beginTransmission(0x36);   //connect to the sensor
    Wire.write(0x0B);               //figure 21 - register map: Status: MD ML MH
    Wire.endTransmission();         //end transmission
    Wire.requestFrom(0x36, 1);      //request data from the sensor

    //read data when it becomes available
    while(Wire.available() == 0);
    magnet_status = Wire.read();

    //give error indicator if failed
    if(((magnet_status & 32) != 32) && (first_attempt == 1)){
      Serial.print("Magnet positioning invalid. Magnet status: ");
      Serial.println(magnet_status, BIN);
      pixels.fill(pixels.Color(BRIGHT, BRIGHT/4, 0), 0);      //orangle lights indicate encoder magnet positioning error
      pixels.show();
      first_attempt = 0;
    }
  }
  
  Serial.println("Encoder magnet found!");
  pixels.clear();
  pixels.show();
}


//get shaft angle position from encoder
void getAngle(){  
  int low_byte;
  int high_byte;
  int angle_raw;
  
  //get bits 0-7 of raw angle
  Wire.beginTransmission(0x36); //connect to the sensor
  Wire.write(0x0D);             //figure 21 - register map: Raw angle (7:0)
  Wire.endTransmission();       //end transmission
  Wire.requestFrom(0x36, 1);    //request data from the sensor

  //read data when it becomes available
  while(Wire.available() == 0);
  low_byte = Wire.read();
 
  //get bits 8-11 of raw angle
  Wire.beginTransmission(0x36); //connect to the sensor
  Wire.write(0x0C);             //figure 21 - register map: Raw angle (11:8)
  Wire.endTransmission();       //end transmission
  Wire.requestFrom(0x36, 1);    //request data from the sensor
  
  while(Wire.available() == 0);  
  high_byte = Wire.read();

  //need to combine the low and high bytes into one 12-bit number
  //therefore, must shift the high byte 8 bits to the left
  high_byte = high_byte << 8;

  //combine low and high bytes
  angle_raw = high_byte | low_byte;

  //map angle to 200 degree rotation (reverse so that encoder direction matches stepper direction)
  cur_angle = map(angle_raw, 0, 4096, 200, 0) - UPRIGHT;
  if(cur_angle < 0){
    cur_angle += 50;
  }

  //srt testing  
  Serial.print("Current location: ");
  Serial.print(cur_angle);
  Serial.println(" degrees (out of 200)");
}


//calculate delay time between pitches based on poteniometer value
void calcDelayTime(){    
  //read potentiometer value
  pot_val = analogRead(POT_PIN);

  //potentiometer has non-linear mapping, so hardcode cutoffs
  if(pot_val <= 40){
    delay_time = map(pot_val, 0, 40, 5000, 6000);
  }
  else if(pot_val <= 125){
    delay_time = map(pot_val, 40, 125, 6000, 7000);
  }
  else if(pot_val <= 280){
    delay_time = map(pot_val, 125, 280, 7000, 8000);
  }
  else if(pot_val <= 715){
    delay_time = map(pot_val, 280, 715, 8000, 9000);
  }
  else{
    delay_time = map(pot_val, 715, 1023, 9000, 10000);
  }

  Serial.print("Potentiometer delay time: ");
  Serial.println(delay_time);

  //LED animation starts with 5 seconds remaining until pitch
  ms_before_led = delay_time - 5000;
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

  //run light animation for the 5 seconds before pitching
  timingAnimation();
    
  //adjust amount to be rotated based on position of cam
  getAngle();
  
  //move 50, less however much cam has over-rotated
  turn_distance = 50 - (cur_angle % 50);

  //if turn distance is less than 10, cam likely got pushed backwards, so need to move extra
  if(turn_distance < 10){
    turn_distance += 50;
  }

  //srt testing
  Serial.print("Turning:");
  Serial.print(turn_distance);
  Serial.println(" degrees (out of 200)");
  Serial.println("-------------------");

  //check again to make sure we haven't pressed the button
  if(false == active){
    return;
  }

  //flash green right as we are pitching
  pixels.fill(pixels.Color(0, BRIGHT, 0), 0);
  pixels.show();
  
  //turn stepper motor 90 degrees, accounting for over-rotated cam
  my_motor->step(turn_distance, BACKWARD, DOUBLE);
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
 *  - mess around with stepper motor speed (slower may catch a new baseball better) - 20rpms seems better when plugged into wall
 *  - add a power switch

 *  - add capability for the users to set number of pitches? - don't worry about that for now
 *  - improve animation?
 *  - how bright do we want the light to be
 *  - add startup light show
 */
