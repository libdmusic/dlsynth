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
       const std::string &information);
  const std::string &name() const;
  const std::string &subject() const;
  const std::string &author() const;
  const std::string &copyright() const;
  const std::string &information() const;

  static Info read(riffcpp::Chunk &chunk);
};
} // namespace DLSynth

#endif