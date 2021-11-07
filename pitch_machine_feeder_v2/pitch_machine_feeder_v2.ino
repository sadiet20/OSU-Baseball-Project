#include <Wire.h>
#include <Adafruit_MotorShield.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


//FUNCTION PROTOTYPES

bool pitching();
bool timingAnimation();
void buttonPress();
void remotePress();


//VARIABLES

//button data
#define BUTTON_PIN 2
#define REMOTE_PIN 3

//time since button was last pressed
//used volatile so that we can edit it within an interrupt
volatile unsigned long button_pressed = 0;

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


//create stepper motor with 200 steps per revolution on port 2
//M1 and M2 go to port 1, M3 and M4 go to port 2
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);

//number of baseballs the device holds
#define MAX_PITCHES 10

//number of pitches thrown
int pitchCount = 0;

//time in between pitches
long delayTime = 0;

//time to wait before starting pitching
#define STARTING_DELAY 0    //30*1000;

//currently throwing pitches
//must be volatile so that it can be altered inside interrupt
volatile bool active = false;


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
}

void loop() {
  //read potentiometer value
  //convert analog (0-1023) to digital (0-255)
  potVal = analogRead(POT_PIN);
  potVal = map(potVal, 0, 1023, 0, 255);

  //calculate the delay time to be between 5 and 10 seconds (depending on where the potentiometer is set)
  delayTime = potVal*(1000)/51 + 5000;

  //LED animation starts with 5 seconds remaining until pitch
  msBeforeLed = delayTime - 5000;
  
  //turn stepper motor maxPitches times (or until the off button is pressed)
  if (active == true && pitchCount!=MAX_PITCHES){
    active = pitching();
  }

  //clear lights and reset count if not pitching
  if(false == active){
    pitchCount = 0;
    pixels.clear();
    pixels.show();
  }

/*
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
  Serial.println(digitalRead(BUTTON_PIN));
*/
}


//executes one pitch with lights
//if at any point 'active' changes to false, immediately returns false
//if successful execution and not the last pitch, returns true
bool pitching(){
  bool cont;
  
  //delay before first pitch and set LEDs to red
  if(pitchCount == 0){
    pixels.fill(pixels.Color(BRIGHT, 0, 0), 0);
    pixels.show();
    for(int i=0; i<100; i++){
      if(false == active){
        return false;
      }
      delay(STARTING_DELAY/100);
    }
  }

  //delay before the 5-second animation
  for(int i=0; i<50; i++){
    if(false == active){
      return false;
    }
    delay(msBeforeLed/50);
  }

  //run light animation for the 5 seconds before pitching
  cont = timingAnimation();

  if(false == cont){
    return false;
  }

  //check again to make sure we haven't pressed the button
  if(false == active){
    return false;
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

  return active;
}


//5-second animation for timing
//returns false if active goes to false or true if active stays true the entire time
bool timingAnimation(){
  for(int i=0; i<NUM_PIXELS; i++){
    if(false == active){
      return false;
    }
    pixels.setPixelColor(i, pixels.Color((BRIGHT + BRIGHT/2), BRIGHT, 0));
    pixels.show();
    delay(MS_BETWEEN_LED);
  }
  return true;
}


//if the button is pressed, toggle the active variable
void buttonPress(){
  //ignore interrupt if the button was pressed within the last half second (ignore double signals)
  if(millis() - button_pressed < 500){
    return;
  }
  button_pressed = millis();
  Serial.println("Button has been pressed.");
  active = !active;
}


void remotePress(){
  Serial.println("Remote signal detected");
  buttonPress();
}

/*
 * Future tasks
 *  - add release() after each movement to catch the next baseball?
 *  - mess around with stepper motor speed (slower may catch a new baseball better)
 *  - why is the remote control not always being detected? -- probably need remote with higher range (currently only 25 feet)
 *  - add fan
 *  - add a power switch
 *  
 *  - how long should the starting delay be?
 *  - add capability for the users to set number of pitches? - don't worry about that for now
 *  - improve animation?
 *  - how bright do we want the light to be
 *  - add startup light show
 * /
 */
