#include "Info.hpp"
#include "CommonFourCCs.hpp"
#include <cassert>
#include <vector>

using namespace DLSynth;

Info::Info(const std::string &name, const std::string &subject,
           const std::string &author, const std::string &copyright,
           const std::string &information) noexcept
  : m_name(name)
  , m_subject(subject)
  , m_author(author)
  , m_copyright(copyright)
  , m_information(information) {}

static std::string readString(riffcpp::Chunk &c) {
  std::vector<char> data;
  data.resize(c.size());
  c.read_data(data.data(), data.size());
  return std::string(data.data(), data.size());
}

Info Info::readChunk(riffcpp::Chunk &chunk) {
  assert(chunk.id() == riffcpp::list_id && chunk.type() == INFO_id);

  std::string name, subject, author, copyright, information;
  for (auto child : chunk) {
    if (child.id() == INAM_id) {
      name = readString(child);
    } else if (child.id() == ISBJ_id) {
      subject = readString(child);
    } else if (child.id() == IENG_id) {
      author = readString(child);
    } else if (child.id() == ICOP_id) {
      copyright = readString(child);
    } else if (child.id() == ICMT_id) {
      information = readString(child);
    }
  }

  return Info(name, subject, author, copyright, information);
}