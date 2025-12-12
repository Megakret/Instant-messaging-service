// USELESS FOR NOW. I will try using names of pipes and open fstreams
#include <transport/PipeTransport.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace transport {
const int kFullPermission = 0600;
PipeTransport::PipeTransport(const std::string &name, PipeFlags flags,
                             ErrCreation &err_ref)
    : filename_(name), flags_(flags) {
  if (Write & flags && Read & flags) {
    err_ref = ErrCreation{kErrOpenReadAndWrite, 0};
  }
  if (Create & flags) {
    int status = mkfifo(name.c_str(), kFullPermission);
    if (status == -1) {
      err_ref = ErrCreation{kErrCreatePipe, errno};
      return;
    }
  }
}
ErrSend PipeTransport::Send(std::span<const char> buffer) const {
  if (!(flags_ & Write)) {
    return ErrSend{kNoPermissions, errno, 0};
  }
  auto [stream, err] = StartStream();
  if (err.error_code == kErrOpenPipe) {
    return ErrSend{kErrOpenPipe, err._errno};
  }
  return stream.Send(buffer);
}
std::pair<int, ErrReceive>
PipeTransport::Receive(std::span<char> buffer) const {
  if (!(flags_ & Read)) {
    return std::make_pair(0, ErrReceive{kNoPermissions, errno});
  }
  auto [stream, err] = StartStream();
  if (err.error_code == kErrOpenPipe) {
    return std::make_pair(0, ErrReceive{kErrOpenPipe, err._errno});
  }
  return stream.Receive(buffer);
}

PipeStream::PipeStream(int pipe_fd) : pipe_fd_(pipe_fd) {}
PipeStream::PipeStream(PipeStream &&stream) : pipe_fd_(stream.pipe_fd_) {
  stream.pipe_fd_ = -1;
}
ErrSend PipeStream::Send(std::span<const char> buffer) const {
  int written = write(pipe_fd_, buffer.data(), buffer.size());
  if (written == -1) {
    return ErrSend{kErrWriteFailed, errno, 0};
  }
  if (written < buffer.size()) {
    return ErrSend{kErrPartialWrite, 0, static_cast<std::size_t>(written)};
  }
  return ErrSend{kSuccess, 0, 0};
}

std::pair<int, ErrReceive> PipeStream::Receive(std::span<char> buffer) const {
  int read_bytes = read(pipe_fd_, buffer.data(), buffer.size());

  if (read_bytes == -1) {
    return std::make_pair(-1, ErrReceive{kErrReadFailed, errno});
  }
  return std::make_pair(read_bytes, ErrReceive{kSuccess, 0});
}
PipeStream::~PipeStream() {
  if (pipe_fd_ != -1) {
    close(pipe_fd_);
  }
}
std::pair<PipeStream, ErrStartStream> PipeTransport::StartStream() const {
  int pipe_fd;
  if (flags_ & Read) {
    pipe_fd = open(filename_.c_str(), O_RDONLY);
  }
  if (flags_ & Write) {
    pipe_fd = open(filename_.c_str(), O_WRONLY);
  }
  if (pipe_fd == -1) {
    return std::make_pair(PipeStream(pipe_fd),
                          ErrStartStream{kErrOpenPipe, errno});
  }
  return std::make_pair(PipeStream(pipe_fd), ErrStartStream{kSuccess});
}
PipeTransport::~PipeTransport() {}
} // namespace transport
