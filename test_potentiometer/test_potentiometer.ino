int pot_pin = A0;   //can also use 0 or 14
int potVal = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  potVal = analogRead(pot_pin);

  Serial.print("potVal original: ");
  Serial.println(potVal);
  
  potVal = map(potVal, 0, 1023, 0, 255);

  Serial.print("potVal after: ");
  Serial.println(potVal);
  Serial.println();

  delay(2000);

}
