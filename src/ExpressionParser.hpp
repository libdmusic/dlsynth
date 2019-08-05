#ifndef EXPRESSIONPARSER_HPP
#define EXPRESSIONPARSER_HPP

#include <cstdint>
#include <memory>
#include <riffcpp.hpp>
#include <vector>

class QueryMap;

namespace DLSynth {
/// Executes DLS condition expressions, expressed in Reverse Polish Notation
class ExpressionParser final {
  std::unique_ptr<QueryMap> m_queryMap;

public:
  ExpressionParser(std::uint32_t sampleRate, std::uint32_t memorySize);
  ~ExpressionParser();

  /// Executes an expression and returns its result
  bool execute(const std::vector<char> &data) const;

  /// Executes the expression contained in the chunk and returns its result
  bool execute(riffcpp::Chunk &chunk) const;
};
} // namespace DLSynth

#endif