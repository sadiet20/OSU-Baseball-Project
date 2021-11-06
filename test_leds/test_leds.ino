#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


//digital pin on the Arduino that is connected to the NeoPixels
#define LED_PIN 15

//how many NeoPixels are attached to the Arduino
#define NUM_PIXELS 8

Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN);


void setup() {
  //initilize NeoPixel strip object
  pixels.begin();
  pixels.fill(pixels.Color(255, 0, 0), 0);    //sets all pixels to red
  pixels.show();
}

void loop() {
  // put your main code here, to run repeatedly:

}
