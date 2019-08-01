#ifndef MODMATRIX_HPP
#define MODMATRIX_HPP
#include "../Articulator.hpp"
#include <vector>

namespace DLSynth {
namespace Synth {
  class ObservableSignal;
  class SignalObserver {
    friend ObservableSignal;

  public:
    virtual ~SignalObserver();

  protected:
    virtual void sourceChanged() = 0;
  };

  class ObservableSignal {
    struct impl;

    impl *pimpl;

  public:
    ObservableSignal();
    virtual ~ObservableSignal();
    void subscribe(SignalObserver *observer);
    void unsubscribe(SignalObserver *observer);
    void resetSubscribers();

  protected:
    void valueChanged();
  };

  class SignalSource : public ObservableSignal {
    struct impl;

    impl *pimpl;

  public:
    SignalSource();
    SignalSource(SignalSource &&);
    ~SignalSource() override;

    SignalSource &operator=(float value);
    SignalSource &operator=(const SignalSource &) = delete;

    operator float() const;

    void value(float v);
    float value() const;
  };

  class SignalDestination : public ObservableSignal, public SignalObserver {
    struct impl;

    impl *pimpl;

  public:
    SignalDestination();
    SignalDestination(SignalDestination &&);
    ~SignalDestination() override;

    SignalDestination &operator=(const SignalDestination &) = delete;

    void addConnection(const SignalSource &source,
                       const TransformParams &srcTransform,
                       const SignalSource &control,
                       const TransformParams &ctrlTransform, float scale);
    void resetConnections();
    float value();
    operator float();

    float asFreq();
    float asSecs();
    float asGain();

  protected:
    virtual void sourceChanged() override;
  };
} // namespace Synth
} // namespace DLSynth

#endif