#include "Error.hpp"

using namespace DLSynth;

Error::Error(const std::string &message, ErrorCode code)
  : std::runtime_error(message), m_code(code) {}

ErrorCode Error::code() const { return m_code; }