#include "Kellegous_TempGrad.h"

Kellegous_TempGrad::Kellegous_TempGrad(uint8_t pin, uint8_t cap)
    : pin_(pin), cap_(cap), len_(0), idx_(0), lst_(1024.0), samples_(0) {
  samples_ = (float*)malloc(sizeof(uint8_t) * cap);
  memset(samples_, 0, sizeof(float) * cap);
#ifdef KELLEGOUS_TEMPGRAD_MOCK_TEMPS
  mck_ = 0;
#endif
}

Kellegous_TempGrad::~Kellegous_TempGrad() {
  free(samples_);
}

// static
float Kellegous_TempGrad::GetTempFromVolt(uint16_t v) {
  v *= 5;
  float tc = (v/1024.0 - 0.5) * 100;
  return (tc * 9.0 / 5.0) + 32.0;
}

uint16_t Kellegous_TempGrad::Read() {
#ifdef KELLEGOUS_TEMPGRAD_MOCK_TEMPS
  uint8_t buf[2];
  if (Serial.available() >= 2 && Serial.readBytes((char*)buf, 2) == 2) {
    mck_ = buf[0]<<8 | buf[1];
  }
  
  return mck_;
#else
  return analogRead(pin_);
#endif
}

bool Kellegous_TempGrad::Update(float* temp, float* grad) {
  *grad = 0.0;

  float curr = GetTempFromVolt(Read());
  float last = lst_;
  lst_ = curr;
  
  *temp = curr;
  
  // was there anything useful in last?
  if (last > 512.0) {
    return 0;
  }
  
  AddSample(curr - last);
  
  // is the buffer full?
  if (len_ < cap_) {
    return 0;
  }

  *grad = ComputeGrad();
  return 1;
}

void Kellegous_TempGrad::AddSample(float dt) {
  samples_[idx_] = dt;
  idx_ = (idx_ + 1) % cap_;
  if (len_ < cap_) {
    len_++;
  }
}

float Kellegous_TempGrad::ComputeGrad() {
  float sum = 0.0;
  for (int i = 0; i < len_; i++) {
    sum += samples_[i];
  }
  return sum / len_;
}

void Kellegous_TempGrad::Dump() {
  Serial.print("cap_ = "); Serial.println(cap_);
  Serial.print("len_ = "); Serial.println(len_);
  Serial.print("idx_ = "); Serial.println(idx_);
  for (int i = 0; i < cap_; i++) {
    Serial.print("samples_["); Serial.print(i); Serial.print("] = "); Serial.println(samples_[i]);
  }
}
