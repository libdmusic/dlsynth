#include "Transform.hpp"
#include <cmath>

using namespace DLSynth::Synth;

Transform::Transform(bool invert, bool bipolar)
  : m_invert(invert), m_bipolar(bipolar) {}

Transform::~Transform() = default;

bool Transform::invert() const { return m_invert; }
bool Transform::bipolar() const { return m_bipolar; }

float Transform::operator()(float input) const {
  if (m_invert) {
    input = 1.f - input;
  }
  return execute(input);
}

class LinearTransform : public Transform {
public:
  LinearTransform(bool invert, bool bipolar) : Transform(invert, bipolar) {}
  virtual ~LinearTransform() override = default;

protected:
  float execute(float value) const override {
    float result = transformFunction(value);
    if (bipolar()) {
      return 2.f * result - 1.f;
    } else {
      return result;
    }
  }

  virtual float transformFunction(float value) const { return value; }
};

class SwitchTransform final : public LinearTransform {
public:
  SwitchTransform(bool invert, bool bipolar)
    : LinearTransform(invert, bipolar) {}
  ~SwitchTransform() override = default;

protected:
  float transformFunction(float value) const override {
    return value < .5f ? 0.f : 1.f;
  }
};

inline float sgn(float x) {
  if (x < 0)
    return -1;
  else
    return 1;
}

class ConcaveTransform : public Transform {
public:
  ConcaveTransform(bool invert, bool bipolar) : Transform(invert, bipolar) {}
  virtual ~ConcaveTransform() = default;

protected:
  float execute(float input) const override {
    if (bipolar()) {
      float value = 2.f * input - 1.f;
      return sgn(value) * transformFunction(std::abs(value));
    } else {
      return transformFunction(input);
    }
  }

  virtual float transformFunction(float value) const {
    static float threshold = 1.f - std::pow(10.f, -12.f / 5.f);
    if (value > threshold) {
      return 1.f;
    } else {
      return -(5.f / 12.f) * std::log10(1.f - value);
    }
  }
};

class ConvexTransform : public ConcaveTransform {
public:
  ConvexTransform(bool invert, bool bipolar)
    : ConcaveTransform(invert, bipolar) {}
  ~ConvexTransform() = default;

protected:
  float transformFunction(float value) const override {
    static float threshold = std::pow(10.f, -12.f / 5.f);
    if (value < threshold) {
      return 0;
    } else {
      return 1.f + (5.f / 12.f) * std::log10(value);
    }
  }
};

std::unique_ptr<Transform> Transform::create(DLSynth::TransformType type,
                                             bool invert, bool bipolar) {
  switch (type) {
  case TransformType::None:
    return std::make_unique<LinearTransform>(invert, bipolar);
  case TransformType::Switch:
    return std::make_unique<SwitchTransform>(invert, bipolar);
  case TransformType::Concave:
    return std::make_unique<ConcaveTransform>(invert, bipolar);
  case TransformType::Convex:
    return std::make_unique<ConvexTransform>(invert, bipolar);
  }
}