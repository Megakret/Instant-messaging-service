#pragma once

#include <span>
#include <string>

namespace transport {
const std::size_t kBufferSize = 1024;
const int kSuccess = 0;
const int kErrCreatePipe = 1;
const int kErrOpenPipe = 2;
const int kErrOpenReadAndWrite = 3;
const int kErrWriteFailed = 4;
const int kErrPartialWrite = 5;
const int kErrReadFailed = 6;
const int kNoPermissions = 7;
const int kErrTimeout = 8;
const int kErrSystem = 9;
enum PipeFlags {
  Create = 1,
  Write = 1 << 1,
  Read = 1 << 2,
  Nonblock = 1 << 3,
};
struct ErrCreation {
  int error_code; // Err constants are here
  int _errno;     // If syscall has failed
};
struct ErrReceive {
  int error_code; // Err constants are here
  int _errno;     // If syscall has failed
};
struct ErrSend {
  int error_code; // Err constants are here
  int _errno;     // If syscall has failed
  std::size_t written_bytes;
};
struct ErrStartStream {
  int error_code; // Err constants are here
  int _errno;     // If syscall has failed
};
class PipeStream {
public:
  PipeStream(int pipe_fd);
  PipeStream(const PipeStream &) = delete;
  PipeStream(PipeStream &&);
  PipeStream &operator=(PipeStream &&);
  ErrSend Send(std::span<const char> buffer) const;
  std::pair<int, ErrReceive> Receive(std::span<char> buffer) const;
  ~PipeStream();

private:
  int pipe_fd_;
};
class PipeTransport {
public:
  PipeTransport(const std::string &name, PipeFlags flags, ErrCreation &err_ref);
  ErrSend Send(std::span<const char> buffer) const;
  std::pair<int, ErrReceive>
  Receive(std::span<char> buffer) const; // Returns bytes read
  std::pair<PipeStream, ErrStartStream> StartStream() const;
  ~PipeTransport();

private:
  const std::string filename_;
  const PipeFlags flags_;
};
inline PipeFlags operator|(PipeFlags a, PipeFlags b) {
  return static_cast<PipeFlags>(static_cast<int>(a) | static_cast<int>(b));
}
} // namespace transport
