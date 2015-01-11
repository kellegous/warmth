#ifndef KELLEGOUS_TEMPGRAD_H_
#define KELLEGOUS_TEMPGRAD_H_

#define KELLEGOUS_TEMPGRAD_MOCK_TEMPS

#include <inttypes.h>
#if ARDUINO >= 100
#include "Arduino.h"
#else
extern "C" {
#include "WConstants.h"
}
#endif

class Kellegous_TempGrad {
 public:
  Kellegous_TempGrad(uint8_t pin, uint8_t size);
  ~Kellegous_TempGrad();
  bool Update(float* temp, float* grad);
  void Dump();
 private:
  static float GetTempFromVolt(uint16_t v);
  void AddSample(float dt);
  float ComputeGrad();
  uint16_t Read();
  uint8_t pin_;
  uint8_t len_;
  uint8_t cap_;
  uint8_t idx_;
  float lst_;
  float* samples_;
#ifdef KELLEGOUS_TEMPGRAD_MOCK_TEMPS
  uint16_t mck_;
#endif
};

#endif //KELLEGOUS_TEMPGRAD_H_
