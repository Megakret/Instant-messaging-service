#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

#include <main_handler_loop.hpp>

const std::string_view kLogFilename = "server.log";
const std::chrono::seconds kPostponeTimeout(30);

int main() {
  int fd = open(kLogFilename.data(), O_WRONLY | O_CREAT, 0600);
  if (fd == -1) {
    std::cerr << "Failed to open log file: " << strerror(errno) << '\n';
    return -1;
  }
  int res = dup2(fd, STDOUT_FILENO);
  if (res == -1) {
    std::cerr << "Failed to dup2: " << strerror(errno) << '\n';
    return -1;
  }
  main_handler_loop(kPostponeTimeout);
}
