#include <handlers/handle.hpp>

namespace handlers {
std::expected<Metadata, ErrReadMd> ReadMetadata(transport::PipeStream &stream) {
  std::array<char, sizeof(Metadata)> buf;
  auto read_bytes = stream.Receive(buf);
  if (!read_bytes) {
    return std::unexpected(ErrReadMd::SystemError);
  }
  if (read_bytes == 0) {
    return std::unexpected(ErrReadMd::Eof);
  }
  if (*read_bytes < buf.size()) {
    return std::unexpected(ErrReadMd::PartialRead);
  }
  return *reinterpret_cast<const Metadata *>(buf.data());
}
} // namespace handlers
