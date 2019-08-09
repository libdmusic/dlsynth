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
  const std::string &name() const noexcept;
  const std::string &subject() const noexcept;
  const std::string &author() const noexcept;
  const std::string &copyright() const noexcept;
  const std::string &information() const noexcept;

  static Info readChunk(riffcpp::Chunk &chunk);
};
} // namespace DLSynth

#endif