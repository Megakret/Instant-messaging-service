//USELESS FOR NOW. I will try using names of pipes and open fstreams
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
ErrSend PipeTransport::Send(std::span<const char> buffer) {
  if (!(flags_ & Write)) {
    return ErrSend{kNoPermissions, errno, 0};
  }
  int pipe_fd_ = open(filename_.c_str(), O_WRONLY);
  if (pipe_fd_ == -1) {
    return ErrSend{kErrOpenPipe, errno, 0};
  }
  int written = write(pipe_fd_, buffer.data(), buffer.size());
  if (written == -1) {
    return ErrSend{kErrWriteFailed, errno, 0};
  }
  if (written < buffer.size()) {
    return ErrSend{kErrPartialWrite, 0};
  }
  return ErrSend{kSuccess, 0, 0};
}
std::pair<int, ErrReceive> PipeTransport::Receive(std::span<char> buffer) {
  if (!(flags_ & Read)) {
    return std::make_pair(0, ErrReceive{kNoPermissions, errno});
  }
  int pipe_fd_ = open(filename_.c_str(), O_RDONLY);
  if (pipe_fd_ == -1) {
    return std::make_pair(-1, ErrReceive{kErrOpenPipe, errno});
  }
  int read_bytes = read(pipe_fd_, buffer.data(), buffer.size());
  if (read_bytes == -1) {
    return std::make_pair(-1, ErrReceive{kErrReadFailed, errno});
  }
  return std::make_pair(read_bytes, ErrReceive{kSuccess, 0});
}
PipeTransport::~PipeTransport() {}
} // namespace transport
