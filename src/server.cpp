#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <main_handler_loop.hpp>

const std::string_view kLogFilename = "server.log";
const std::chrono::seconds kPostponeTimeout(30);
void Shutdown(int signum) {
  std::cout << "Shutdown\n";
  std::cout.flush();
  exit(0);
}
int main() {
  int fd = open(kLogFilename.data(), O_WRONLY | O_CREAT, 0600);
  if (fd == -1) {
    std::cerr << "Failed to open log file: " << strerror(errno) << '\n';
    return -1;
  }
  int res = dup2(fd, STDOUT_FILENO);
  close(fd);
  if (res == -1) {
    std::cerr << "Failed to dup2: " << strerror(errno) << '\n';
    return -1;
  }
  struct sigaction sigact;
  sigact.sa_handler = &Shutdown;
  sigact.sa_flags &= ~SA_RESTART;
  int status = sigaction(SIGINT, &sigact, NULL);
  if (status == -1) {
    std::cerr << "Error when establishing signal handler\n";
    return -1;
  }
  main_handler_loop(kPostponeTimeout);
}
