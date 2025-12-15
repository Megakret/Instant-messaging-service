#pragma once

#include <expected>
#include <span>
#include <string>

namespace transport {
const std::size_t kBufferSize = 1024;
enum class PipeErr {
	Success,
  CreateFailed,
  OpenFailed,
  WrongFlags,
  WriteFailed,
  ReadFailed,
  NoPermission,
};
enum PipeFlags {
  Create = 1,
  Write = 1 << 1,
  Read = 1 << 2,
};
class PipeStream {
public:
  PipeStream(int pipe_fd);
  PipeStream(const PipeStream &) = delete;
  PipeStream(PipeStream &&);
  PipeStream &operator=(PipeStream &&);
  std::expected<int, PipeErr> Send(std::span<const char> buffer) const;
  std::expected<int, PipeErr> Receive(std::span<char> buffer) const;
  ~PipeStream();

private:
  int pipe_fd_;
};
class PipeTransport {
public:
  PipeTransport(const std::string &name, PipeFlags flags, PipeErr &err_ref);
	std::expected<int, PipeErr> Send(std::span<const char> buffer) const;
  std::expected<int, PipeErr>
  Receive(std::span<char> buffer) const; // Returns bytes read
  std::expected<PipeStream, PipeErr> StartStream() const;
  ~PipeTransport();

private:
  const std::string filename_;
  const PipeFlags flags_;
};
inline PipeFlags operator|(PipeFlags a, PipeFlags b) {
  return static_cast<PipeFlags>(static_cast<int>(a) | static_cast<int>(b));
}
} // namespace transport
