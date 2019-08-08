STRUCT_BEGIN(Range)
FIELD(std::uint16_t, low)
FIELD(std::uint16_t, high)

STRUCT_METHOD(constexpr bool inRange(std::uint16_t value) const noexcept {
  return (value <= high && value >= low) || (low == 0 && high == 0);
})
STRUCT_END(Range)