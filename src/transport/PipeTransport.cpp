#include <transport/PipeTransport.hpp>

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace transport {
constexpr int kFullPermission = 0600;
PipeTransport::PipeTransport(const std::string &name, PipeFlags flags,
                             PipeErr &err_ref)
    : filename_(name), flags_(flags) {
  err_ref = PipeErr::Success;
  if (Write & flags && Read & flags) {
    err_ref = PipeErr::WrongFlags;
  }
  if (Create & flags) {
    int status = mkfifo(name.c_str(), kFullPermission);
    if (status == -1) {
      if (errno == EEXIST) {
        return;
      }
      err_ref = PipeErr::CreateFailed;
      std::cout << "Failed to create pipe: " << strerror(errno) << '\n';
      return;
    }
  }
}
std::expected<int, PipeErr>
PipeTransport::Send(std::span<const char> buffer) const {
  if (!(flags_ & Write)) {
    return std::unexpected(PipeErr::NoPermission);
  }
  auto stream = StartStream();
  if (stream) {
    return stream->Send(buffer);
  } else {
    return std::unexpected(stream.error());
  }
}
std::expected<int, PipeErr>
PipeTransport::Receive(std::span<char> buffer) const {
  if (!(flags_ & Read)) {
    return std::unexpected(PipeErr::NoPermission);
  }
  auto stream = StartStream();
  if (stream) {
    return stream->Receive(buffer);
  } else {
    return std::unexpected(stream.error());
  }
}

PipeStream::PipeStream(int pipe_fd) : pipe_fd_(pipe_fd) {}
PipeStream::PipeStream(PipeStream &&stream) : pipe_fd_(stream.pipe_fd_) {
  stream.pipe_fd_ = -1;
}
std::expected<int, PipeErr>
PipeStream::Send(std::span<const char> buffer) const {
  int written = write(pipe_fd_, buffer.data(), buffer.size());
  if (written == -1) {
    std::cout << "Failed to write data: " << strerror(errno) << '\n';
    return std::unexpected(PipeErr::WriteFailed);
  }
  return written;
}

std::expected<int, PipeErr> PipeStream::Receive(std::span<char> buffer) const {
  int read_bytes = read(pipe_fd_, buffer.data(), buffer.size());
  if (read_bytes == -1) {
    std::cout << "Failed to read data: " << strerror(errno) << '\n';
    return std::unexpected(PipeErr::ReadFailed);
  }
  return read_bytes;
}
PipeStream &PipeStream::operator=(PipeStream &&other) {
  PipeStream moved(std::move(other));
  this->pipe_fd_ = moved.pipe_fd_;
  moved.pipe_fd_ = -1;
  return *this;
}
PipeStream::~PipeStream() {
  if (pipe_fd_ != -1) {
    close(pipe_fd_);
  }
}
std::expected<PipeStream, PipeErr> PipeTransport::StartStream() const {
  int pipe_fd;
  if (flags_ & Read) {
    pipe_fd = open(filename_.c_str(), O_RDONLY);
  }
  if (flags_ & Write) {
    pipe_fd = open(filename_.c_str(), O_WRONLY);
  }
  if (pipe_fd == -1) {
    return std::unexpected(PipeErr::OpenFailed);
  }
  return PipeStream(pipe_fd);
}
PipeTransport::~PipeTransport() {}
} // namespace transport
