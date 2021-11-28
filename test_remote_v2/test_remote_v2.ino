#define PIN 3

int reading;
int old_reading;

volatile int interrupt_reading;
volatile bool interrupt_ran = false;
volatile unsigned int interrupt_count = 0;
volatile long long interrupt_time = 0;

void interrupt();

void setup() {
  Serial.begin(9600);
  Serial.println("Testing remote control");
  
  pinMode(PIN, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(PIN), interrupt, RISING);

  reading = digitalRead(PIN);
  Serial.print(millis());
  Serial.print(": ");
  Serial.println(reading);
  old_reading = reading;
}

void loop() {
  reading = digitalRead(PIN);
  if(reading != old_reading){
    Serial.print(millis());
    Serial.print(": ");
    Serial.println(reading);
    old_reading = reading;
  } 

  if(true == interrupt_ran){
    Serial.print(millis());
    Serial.print(": Interrupt run (");
    Serial.print(interrupt_count);
    Serial.print(") with a reading of ");
    Serial.println(interrupt_reading);
    interrupt_ran = false;
  }
}

//200 millis is a good time for delay on button
//for remote, 200 millis delay and only act on the odd presses (each press triggers interrupt twice)
void interrupt() {
  noInterrupts();
  if(millis() > interrupt_time + 200){ 
    interrupt_count++;
    if(interrupt_count % 2 == 1){
      interrupt_reading = digitalRead(PIN);
      interrupt_ran = true;
      interrupt_time = millis();
    }
  }
  interrupts();
}
