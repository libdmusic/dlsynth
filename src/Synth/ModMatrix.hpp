#ifndef MODMATRIX_HPP
#define MODMATRIX_HPP
#include "../Articulator.hpp"
#include <vector>

namespace DLSynth {
namespace Synth {
  class ObservableSignal;

  /// An object that can subscribe to value change events of other \ref
  /// ObservableSignal s
  class SignalObserver {
    friend ObservableSignal;

  public:
    virtual ~SignalObserver();
    SignalObserver &operator=(SignalObserver &) = delete;

  protected:
    /// Called when the value of one of the \ref ObservableSignal s to which
    /// this object is subscribed has changed value.
    virtual void sourceChanged() = 0;
  };

  /// An object whose change in value can be listened by \ref SignalObserver s
  class ObservableSignal {
    struct impl;

    impl *pimpl;

  public:
    ObservableSignal();
    ObservableSignal(ObservableSignal &&);
    virtual ~ObservableSignal();

    ObservableSignal &operator=(ObservableSignal &) = delete;
    void subscribe(SignalObserver *observer);
    void resetSubscribers();

  protected:
    /// Signals to all the subscribed listeners that the value of this object
    /// changed.
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

    void addConnection(SignalSource &source,
                       const TransformParams &srcTransform,
                       SignalSource &control,
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