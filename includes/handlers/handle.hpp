#include <fstream>
#include <functional>
#include <iostream>

namespace handlers {
template <typename ProtoResponce, typename ProtoRequest>
void HandleRequest(std::fstream& accepting_pipe,
                   std::function<ProtoResponce(ProtoRequest)> handler) {
  ProtoRequest request;

  if (!request.ParseFromIstream(&accepting_pipe)) {
    std::cout << "Failed to accept connection due to parsing error\n";
    return;
  }
  auto responce = handler(request);
  std::fstream user_pipe(request.pipe_path(), std::ios::out | std::ios::binary);
  if (!responce.SerializeToOstream(&user_pipe)) {
    std::cout << "Failed to write responce due to serializing error\n";
  };
}
} // namespace handlers
