#ifndef EXPRESSIONPARSER_HPP
#define EXPRESSIONPARSER_HPP

#include <cstdint>
#include <memory>
#include <vector>

class QueryMap;

namespace DLSynth {
/// Executes DLS condition expressions, expressed in Reverse Polish Notation
class ExpressionParser {
  std::unique_ptr<QueryMap> m_queryMap;

public:
  ExpressionParser(std::uint32_t sampleRate, std::uint32_t memorySize);
  ~ExpressionParser();

  /// Executes an expression and returns its result
  bool execute(const std::vector<std::uint8_t> &data) const;
};
} // namespace DLSynth

#endif