#include <handlers/handle.hpp>

namespace handlers {
std::pair<Metadata, ErrReadMd> ReadMetadata(transport::PipeStream &stream) {
  std::array<char, sizeof(Metadata)> buf;
  auto [read_bytes, err] = stream.Receive(buf);
  if (read_bytes == 0) {
    return std::make_pair(Metadata{}, ErrReadMd{ErrReadMd::Eof});
  }

  if (read_bytes < buf.size()) {
    return std::make_pair(Metadata{}, ErrReadMd{ErrReadMd::Success});
  }
  if (err.error_code != transport::kSuccess) {
    return std::make_pair(Metadata{},
                          ErrReadMd{ErrReadMd::SystemError, err._errno});
  }
  return std::make_pair(*reinterpret_cast<Metadata *>(buf.data()),
                        ErrReadMd{ErrReadMd::Success, 0});
}
} // namespace handlers
