#include <Adafruit_MotorShield.h>


//create stepper motor with 200 steps per revolution on port 1
//M1 and M2 go to port 1, M3 and M4 go to port 2
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);


void setup() {
  //setup serial monitor at 9600 bps
  Serial.begin(9600);
  while(!Serial);
  Serial.println("START AUTOMATED BASEBALL FEEDER");

  Serial.println("init motor");
  
  //initialize the stepper motor with speed 20RPMS
  AFMS.begin();
  
  Serial.println("motor speed");
  myMotor->setSpeed(20);
}

void loop() {
  //wait 1 second
  delay(1000);
  
  Serial.println("Movement");
  
  //turn stepper motor 90 degrees
  myMotor->step(50, BACKWARD, DOUBLE);
}
