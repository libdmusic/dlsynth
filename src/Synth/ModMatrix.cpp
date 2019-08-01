#include "ModMatrix.hpp"
#include "../NumericUtils.hpp"
#include "Transform.hpp"
#include <cmath>

using namespace DLSynth;
using namespace DLSynth::Synth;

struct ObservableSignal::impl {
  std::vector<SignalObserver *> m_subscribers;
};

ObservableSignal::ObservableSignal() : pimpl(new impl()) {}
ObservableSignal::~ObservableSignal() { delete pimpl; }

void ObservableSignal::subscribe(SignalObserver *subscriber) {
  pimpl->m_subscribers.push_back(subscriber);
}

void ObservableSignal::unsubscribe(SignalObserver *subscriber) {
  auto &vector = pimpl->m_subscribers;
  vector.erase(std::find(std::begin(vector), std::end(vector), subscriber));
}

void ObservableSignal::resetSubscribers() {
  auto &vector = pimpl->m_subscribers;
  vector.erase(std::begin(vector), std::end(vector));
}

void ObservableSignal::valueChanged() {
  for (auto &subscriber : pimpl->m_subscribers) {
    subscriber->sourceChanged();
  }
}

SignalObserver::~SignalObserver() = default;

struct SignalSource::impl {
  float m_value;
};

SignalSource::SignalSource() : pimpl(new impl()) {}
SignalSource::SignalSource(SignalSource &&src) : pimpl(src.pimpl) {
  src.pimpl = nullptr;
}

SignalSource::~SignalSource() { delete pimpl; }

SignalSource &SignalSource::operator=(float v) {
  value(v);
  return *this;
}

SignalSource::operator float() const { return value(); }

void SignalSource::value(float v) {
  pimpl->m_value = v;
  valueChanged();
}
float SignalSource::value() const { return pimpl->m_value; }

struct Connection {
  const SignalSource *src, *ctrl;
  std::unique_ptr<DLSynth::Synth::Transform> srcTransform, ctrlTransform;
  float scale;
};

struct SignalDestination::impl {
  std::vector<Connection> m_connections;
  bool m_upToDate = true;
  float m_value = 0.f;

  bool m_freqUpToDate = false;
  float m_freq = 0.f;

  bool m_gainUpToDate = false;
  float m_gain = 0.f;

  bool m_secsUpToDate = false;
  float m_secs = 0.f;
};

SignalDestination::SignalDestination() : pimpl(new impl()) {}
SignalDestination::SignalDestination(SignalDestination &&dst)
  : pimpl(dst.pimpl) {
  dst.pimpl = nullptr;
}
SignalDestination::~SignalDestination() { delete pimpl; }

void SignalDestination::addConnection(
 SignalSource &source, const DLSynth::TransformParams &srcTransform,
 SignalSource &control, const DLSynth::TransformParams &ctrlTransform,
 float scale) {

  Connection conn{&source, &control,
                  Transform::create(srcTransform.type(), srcTransform.invert(),
                                    srcTransform.bipolar()),
                  Transform::create(ctrlTransform.type(),
                                    ctrlTransform.invert(),
                                    ctrlTransform.bipolar()),
                  scale};
  pimpl->m_connections.push_back(std::move(conn));
  pimpl->m_upToDate = false;
  pimpl->m_freqUpToDate = false;
  pimpl->m_gainUpToDate = false;
  pimpl->m_secsUpToDate = false;
  source.subscribe(this);
  control.subscribe(this);
  valueChanged();
}

void SignalDestination::sourceChanged() {
  pimpl->m_upToDate = false;
  pimpl->m_freqUpToDate = false;
  pimpl->m_gainUpToDate = false;
  pimpl->m_secsUpToDate = false;
  valueChanged();
}

float SignalDestination::value() {
  if (!pimpl->m_upToDate) {
    pimpl->m_value = 0.f;
    for (auto &conn : pimpl->m_connections) {
      float source = conn.src->value();
      float control = conn.ctrl->value();
      float srcTransformed = (*conn.srcTransform)(source);
      float ctrlTransformed = (*conn.ctrlTransform)(control);

      pimpl->m_value += srcTransformed * ctrlTransformed * conn.scale;
    }
    pimpl->m_upToDate = true;
  }

  return pimpl->m_value;
}

SignalDestination::operator float() { return value(); }

void SignalDestination::resetConnections() {
  pimpl->m_connections.erase(std::begin(pimpl->m_connections),
                             std::end(pimpl->m_connections));
  pimpl->m_upToDate = false;
  pimpl->m_freqUpToDate = false;
  pimpl->m_gainUpToDate = false;
  pimpl->m_secsUpToDate = false;
  valueChanged();
}

float SignalDestination::asFreq() {
  if (!pimpl->m_freqUpToDate) {
    pimpl->m_freq = centsToFreq(value());
    pimpl->m_freqUpToDate = true;
  }
  return pimpl->m_freq;
}
float SignalDestination::asSecs() {
  if (!pimpl->m_secsUpToDate) {
    pimpl->m_secs = centsToSecs(value());
    pimpl->m_secsUpToDate = true;
  }

  return pimpl->m_secs;
}
float SignalDestination::asGain() {
  if (!pimpl->m_gainUpToDate) {
    pimpl->m_gain = belsToGain(value());
    pimpl->m_gainUpToDate = true;
  }

  return pimpl->m_gain;
}