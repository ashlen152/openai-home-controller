#include <Arduino.h>

void setup() {
  // No need to initialize the RGB LED
  Serial.begin(115200);
  delay(3000);
#ifdef RGB_BUILTIN
  Serial.println("RGB_BUILTIN");
#else
  Serial.println("no RGB_BUILTIN");
#endif
}

// the loop function runs over and over again forever
void loop() {
#ifdef RGB_BUILTIN
  digitalWrite(RGB_BUILTIN, HIGH); // Turn the RGB LED white
  delay(1000);
  digitalWrite(RGB_BUILTIN, LOW); // Turn the RGB LED off
  delay(1000);

  rgbLedWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red
  delay(1000);
  rgbLedWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green
  delay(1000);
  rgbLedWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); // Blue
  delay(1000);
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
  delay(1000);
#endif
}
