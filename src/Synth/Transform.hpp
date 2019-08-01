#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "../Articulator.hpp"
#include <memory>

namespace DLSynth {
namespace Synth {
  class Transform {
    bool m_invert, m_bipolar;

  public:
    virtual ~Transform();
    float operator()(float input) const;

    static std::unique_ptr<Transform> create(TransformType type, bool invert,
                                             bool bipolar);

  protected:
    Transform(bool invert, bool bipolar);
    virtual float execute(float value) const = 0;

    bool invert() const;
    bool bipolar() const;
  };
} // namespace Synth
} // namespace DLSynth

#endif