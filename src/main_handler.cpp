#include <fstream>
#include <iostream>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <handlers/messenger_service.hpp>
#include <protos/main.pb.h>

void main_handler_loop() {
  handlers::MessegingService messeging_service;
  while (true) {
    std::fstream accepting_pipe(kAcceptingPipePath.data(),
                                std::ios::in | std::ios::binary);
    int msg_typeid;
    accepting_pipe >> msg_typeid;
    switch (msg_typeid) {
    case kConnectionMsgID:
      handlers::HandleRequest<messenger::ConnectResponce,
                              messenger::ConnectMessage>(
          accepting_pipe, [&messeging_service](auto req) -> auto {
            return messeging_service.CreateConnection(req);
          });
      break;
      // idk
    case kDisconnectMsgID:
      handlers::HandleRequest<messenger::DisconnectResponce,
                              messenger::DisconnectMessage>(
          accepting_pipe, [&messeging_service](auto req) -> auto {
            return messeging_service.CloseConnection(req);
          });
      break;
    }
  }
}
