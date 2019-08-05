#include "ExpressionParser.hpp"
#include "Structs.hpp"
#include "Uuid.hpp"
#include <array>
#include <functional>
#include <stack>
#include <unordered_map>

using namespace DLSynth;

enum class OpCode : std::uint16_t {
  And = 0x0001,
  Or = 0x0002,
  Xor = 0x0003,
  Add = 0x0004,
  Sub = 0x0005,
  Mul = 0x0006,
  Div = 0x0007,
  LAnd = 0x0008,
  LOr = 0x0009,
  Lt = 0x000A,
  Le = 0x000B,
  Gt = 0x000C,
  Ge = 0x000D,
  Eq = 0x000E,
  Not = 0x000F,
  Const = 0x0010,
  Query = 0x0011,
  Supported = 0x0012
};

void operation(std::stack<uint32_t> &stack,
               std::function<std::uint32_t(std::uint32_t)> f) {
  uint32_t x = stack.top();
  stack.pop();
  stack.push(f(x));
}

void operation(std::stack<uint32_t> &stack,
               std::function<std::uint32_t(std::uint32_t, std::uint32_t)> f) {
  uint32_t x = stack.top();
  stack.pop();
  uint32_t y = stack.top();
  stack.pop();
  stack.push(f(x, y));
}

constexpr Uuid Query_GMInHardware = {
 0x178F2F24, 0xC364, 0x11D1, {0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12}};
constexpr Uuid Query_GSInHardware = {
 0x178F2F25, 0xC364, 0x11D1, {0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12}};
constexpr Uuid Query_XGInHardware = {
 0x178F2F26, 0xC364, 0x11D1, {0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12}};
constexpr Uuid Query_SupportsDLS1 = {
 0x178F2F27, 0xC364, 0x11D1, {0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12}};
constexpr Uuid Query_SupportsDLS2 = {
 0xF14599E5, 0x4689, 0x11D2, {0xAF, 0xA6, 0x00, 0xAA, 0x00, 0x24, 0xd8, 0xB6}};
constexpr Uuid Query_SampleMemorySize = {
 0x178F2F28, 0xC364, 0x11D1, {0xA7, 0x60, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12}};
constexpr Uuid Query_ManufacturersID = {
 0xB03E1181, 0x8095, 0x11D2, {0xA1, 0xEF, 0x00, 0x60, 0x08, 0x33, 0xDB, 0xD8}};
constexpr Uuid Query_ProductID = {
 0xB03E1182, 0x8095, 0x11D2, {0xA1, 0xEF, 0x00, 0x60, 0x08, 0x33, 0xDB, 0xD8}};
constexpr Uuid Query_SamplePlaybackRate = {
 0x2A91F713, 0xA4BF, 0x11D2, {0xBB, 0xDF, 0x00, 0x60, 0x08, 0x33, 0xDB, 0xD8}};

class QueryMap {
  std::unordered_map<Uuid, std::uint32_t> m_map;

public:
  QueryMap(std::uint32_t sampleRate, std::uint32_t memorySize) {
    m_map[Query_GMInHardware] = 0;
    m_map[Query_GSInHardware] = 0;
    m_map[Query_XGInHardware] = 0;
    m_map[Query_SupportsDLS1] = 1;
    m_map[Query_SupportsDLS2] = 1;
    m_map[Query_SampleMemorySize] = memorySize;
    m_map[Query_ManufacturersID] = 0x7D;
    m_map[Query_ProductID] = 0;
    m_map[Query_SamplePlaybackRate] = sampleRate;
  }

  std::uint32_t supports(const Uuid &id) const {
    return (m_map.find(id) == m_map.end()) ? 0 : 1;
  }

  bool query(const Uuid &id) const {
    if (supports(id)) {
      return m_map.at(id);
    } else {
      return 0;
    }
  }
};

ExpressionParser::ExpressionParser(std::uint32_t sampleRate,
                                   std::uint32_t memorySize)
  : m_queryMap(std::make_unique<QueryMap>(sampleRate, memorySize)) {}

bool ExpressionParser::execute(const std::vector<char> &data) const {
  std::stack<uint32_t> stack;
  const char *buf = data.data();
  const char *end = buf + data.size();
  const char *pos = buf;
  while (pos < end) {
    OpCode op;
    pos = StructLoader<OpCode>::readBuffer(buf, end, op);
    switch (op) {
    case OpCode::Add:
      operation(stack, [](auto x, auto y) { return x + y; });
      break;
    case OpCode::And:
      operation(stack, [](auto x, auto y) { return x & y; });
      break;
    case OpCode::Const: {
      std::uint32_t c;
      pos = StructLoader<std::uint32_t>::readBuffer(buf, end, c);
      stack.push(c);
    } break;
    case OpCode::Div:
      operation(stack, [](auto x, auto y) { return x / y; });
      break;
    case OpCode::Eq:
      operation(stack, [](auto x, auto y) { return x == y; });
      break;
    case OpCode::Ge:
      operation(stack, [](auto x, auto y) { return x >= y; });
      break;
    case OpCode::Gt:
      operation(stack, [](auto x, auto y) { return x > y; });
      break;
    case OpCode::LAnd:
      operation(stack, [](auto x, auto y) { return x && y; });
      break;
    case OpCode::LOr:
      operation(stack, [](auto x, auto y) { return x || y; });
      break;
    case OpCode::Le:
      operation(stack, [](auto x, auto y) { return x <= y; });
      break;
    case OpCode::Lt:
      operation(stack, [](auto x, auto y) { return x < y; });
      break;
    case OpCode::Mul:
      operation(stack, [](auto x, auto y) { return x * y; });
      break;
    case OpCode::Not:
      operation(stack, [](auto x) { return !x; });
      break;
    case OpCode::Or:
      operation(stack, [](auto x, auto y) { return x | y; });
      break;
    case OpCode::Sub:
      operation(stack, [](auto x, auto y) { return x - y; });
      break;
    case OpCode::Xor:
      operation(stack, [](auto x, auto y) { return x ^ y; });
      break;
    case OpCode::Query: {
      Uuid query;
      pos = StructLoader<Uuid>::readBuffer(pos, end, query);
      stack.push(m_queryMap->query(query));
    } break;
    case OpCode::Supported: {
      Uuid query;
      pos = StructLoader<Uuid>::readBuffer(pos, end, query);
      stack.push(m_queryMap->supports(query));
    } break;
    }
  }

  return stack.top();
}

bool ExpressionParser::execute(riffcpp::Chunk &chunk) const {
  std::vector<char> data(chunk.size());
  chunk.read_data(data.data(), data.size());
  return execute(data);
}

ExpressionParser::~ExpressionParser() = default;