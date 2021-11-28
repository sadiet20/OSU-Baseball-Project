#define PIN 3

int reading;
int old_reading;

void setup() {
  Serial.begin(9600);
  Serial.println("Testing remote control");
  
  pinMode(PIN, INPUT);

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
}
