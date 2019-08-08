#ifndef INFO_HPP
#define INFO_HPP

#include <riffcpp.hpp>
#include <string>

namespace DLSynth {
class Info {
  std::string m_name, m_subject, m_author, m_copyright, m_information;

public:
  Info(const std::string &name, const std::string &subject,
       const std::string &author, const std::string &copyright,
       const std::string &information) noexcept;
  constexpr const std::string &name() const noexcept { return m_name; }
  constexpr const std::string &subject() const noexcept { return m_subject; }
  constexpr const std::string &author() const noexcept { return m_author; }
  constexpr const std::string &copyright() const noexcept {
    return m_copyright;
  }
  constexpr const std::string &information() const noexcept {
    return m_information;
  }

  static Info readChunk(riffcpp::Chunk &chunk);
};
} // namespace DLSynth

#endif