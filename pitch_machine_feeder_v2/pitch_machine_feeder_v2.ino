#include <Wire.h>
#include <Adafruit_MotorShield.h>

//button data
#define BUTTON_PIN 2

//potentiometer data
#define POT_PIN 0
int potVal = 0;

//create stepper motor with 200 steps per revolution on port 1
//M1 and M2 go to port 1, M3 and M4 go to port 2
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1);

//number of baseballs the device holds
#define MAX_PITCHES 10

//number of pitches thrown
int pitchCount = 0;

//time in between pitches
int delayTime = 0;

//time to wait before starting pitching
#define STARTING_DELAY 0    //30*1000;

//currently throwing pitches
bool active = false;


void setup() {
  //setup serial monitor at 9600 bps
  Serial.begin(9600);
  Serial.println("START AUTOMATED BASEBALL FEEDER");

  //setup input button
  //high==on==pressed low==off==not pressed
  pinMode(BUTTON_PIN, INPUT);

  //initialize the stepper motor with speed 40RPMS
  AFMS.begin();
  myMotor->setSpeed(40); 
}

void loop() {
  //read potentiometer value
  //convert analog (0-1023) to digital (0-255)
  potVal = analogRead(POT_PIN);
  potVal = map(potVal, 0, 1023, 0, 255);

  //calculate the delay time to be between 5 and 10 seconds (depending on where the potentiometer is set)
  delayTime = potVal*(1000)/51 + 5000;

  if(digitalRead(BUTTON_PIN) == LOW){
    active = !active;
    Serial.println("Button press!");
    Serial.print("Active: ");
    Serial.println(active);
  }
  
  //turn stepper motor maxPitches times (or until the off button is pressed)
  if (active == true && pitchCount!=MAX_PITCHES)
  {
    if(pitchCount == 0){
      delay(STARTING_DELAY);
    }


    //turn stepper motor 90 degrees
    myMotor->step(50, BACKWARD, DOUBLE);
    pitchCount++;

    //loop through to check button press
    for(int i=0; i<20; i++){
      delay(delayTime/20);
      if(digitalRead(BUTTON_PIN) == LOW){
        active = false;
        Serial.println("Button press inside active!");
        break;
      }
    }

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
  Serial.println(digitalRead(BUTTON_PIN));
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
