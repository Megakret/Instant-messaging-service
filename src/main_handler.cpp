#include <fstream>
#include <iostream>

#include <config.hpp>
#include <messenger_service.hpp>
#include <protos/main.pb.h>

void main_handler_loop() {
  MessegingService messeging_service;
  while (true) {
    std::fstream accepting_pipe(kAcceptingPipePath.data(),
                                std::ios::in | std::ios::binary);
    int msg_typeid;
    accepting_pipe >> msg_typeid;
    switch (msg_typeid) {
    case kConnectionMsgID:
      messenger::ConnectMessage request;
      if (!request.ParseFromIstream(&accepting_pipe)) {
        std::cout << "Failed to accept connection due to parsing error\n";
        break;
      }
      auto responce = messeging_service.CreateConnection(request);
      std::fstream user_pipe(request.pipe_path(),
                             std::ios::out | std::ios::binary);
      if (!responce.SerializeToOstream(&user_pipe)) {
        std::cout << "Failed to write responce due to serializing error\n";
      };
      break;
      // idk
    }
  }
}
