#include "Kellegous_TempGrad.h"

const int TEMP_PIN = 0;

unsigned long last_temp_at;

Kellegous_TempGrad temp(TEMP_PIN, 8);

float baseline;

float ComputeTemp(float v) {
  float tc = (v/1024.0 - 0.5) * 100;
  return (tc * 9.0 / 5.0) + 32.0;  
}

void setup() {
  last_temp_at = millis();
  Serial.begin(9600);

  float tmp, grd;
  for (int i = 0; i < 16; i++) {
    temp.Update(&tmp, &grd);
    baseline += tmp;
  }
  baseline /= 16;
  
  Serial.print("baseline = "); Serial.println(baseline);
}

void loop() {
  unsigned long t = millis();
  if (t - last_temp_at < 1000) {
    return;
  }
  
  last_temp_at = t;

  float tmp;
  float grd;
  if (!temp.Update(&tmp, &grd)) {
    return;
  }
  Serial.print(tmp); Serial.print(", "); Serial.println(grd);
  // temp.Dump();
}
