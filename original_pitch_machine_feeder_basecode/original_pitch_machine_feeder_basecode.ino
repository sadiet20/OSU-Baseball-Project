#include <Wire.h>
#include <Adafruit_MotorShield.h>

int pitchCount = 0; // count of current number of pitches

//button implementation
int buttonPin = 2;
bool Run = false;
bool buttonPress = false;


//potentiometer implementation
int potPin = 0;
int potVal = 0;

//unused
//int maxPitches = 3; // # of pitches until the loop ends

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1);


void setup() {
  Serial.begin(9600);  // set up Serial library at 9600 bps
  Serial.println("START AUTOMATED BASEBALL FEEDER");
  Serial.println(" ");

  //button setup
  pinMode(buttonPin, INPUT_PULLUP);


  //read potentiometer val
  potVal = analogRead(potPin);
  potVal = map(potVal, 0, 1023, 0, 100);


  //unused interrupt
  // attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPress, CHANGE);

  AFMS.begin();  // create with the default frequency 1.6KHz
  myMotor->setSpeed(40); // motor speed (RPMs)
}

void loop() {

  //read potentiometer value
  potVal = analogRead(potPin);
  potVal = map(potVal, 0, 1023, 0, 255);

  Serial.print("potVal: ");
  Serial.print(potVal);
  Serial.print("   ");
  Serial.print("delay: ");
  Serial.println(30 + potVal * 0.13);

  //button checks
  //check if button is pressed
  if (digitalRead(buttonPin) == LOW)
    buttonPress = true;

  //if button has been pressed & released & loop isnt running, start loop
  if (buttonPress == true && digitalRead(buttonPin) == HIGH && Run == false) {
    Run = true;
    buttonPress = false;
  }

  //if button has been pressed & released & loop is running, stop loop
  if (buttonPress == true && digitalRead(buttonPin) == HIGH && Run == true) {
    Run = false;
    buttonPress = false;
  }


  //stepper is rotating
  //if loop started, run loop
  if (Run == true && digitalRead(buttonPin) == HIGH)
  {
    myMotor->step(50, BACKWARD, DOUBLE);
    pitchCount++;

    //instead of one 'delay(1000)' use a for loop to split up delay, so buttonPress can be checked for
    //for (int i = 0; i < (potVal + 20); i++) {
    for (int i = 0; i < 150; i++) {
      //set min delay btwn loops & add time based on potentiometer value
      delay(30 + potVal * 0.13); //ranges from 5 sec to 10 sec btwn loops
      //check if button is pressed
      if (digitalRead(buttonPin) == LOW)
        buttonPress = true;
    }
  }
}
