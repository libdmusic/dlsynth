#ifndef EXPRESSIONPARSER_HPP
#define EXPRESSIONPARSER_HPP

#include <cstdint>
#include <memory>
#include <vector>

class QueryMap;

namespace DLSynth {
class ExpressionParser {
  std::unique_ptr<QueryMap> m_queryMap;

public:
  ExpressionParser(std::uint32_t sampleRate, std::uint32_t memorySize);
  ~ExpressionParser();

  bool execute(const std::vector<std::uint8_t> &data) const;
};
} // namespace DLSynth

#endif