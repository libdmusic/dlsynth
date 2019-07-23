#ifndef ERROR_HPP
#define ERROR_HPP

#include <dlsynth.h>
#include <stdexcept>
#include <string>

namespace DLSynth {
enum class ErrorCode : int {
#define DLSYNTH_ERROR(name, value) name = value,
#include <dlsynth_errors.h>
#undef DLSYNTH_ERROR
};

class Error : public std::runtime_error {
  ErrorCode m_code;

public:
  Error(const std::string &message, ErrorCode code);
  ErrorCode code() const;
};
} // namespace DLSynth

#endif