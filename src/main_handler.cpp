#include <iostream>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <handlers/messenger_service.hpp>
#include <protos/main.pb.h>

void main_handler_loop() {
  handlers::MessegingService messeging_service;
  transport::PipeErr err;
  transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                       transport::Read | transport::Create,
                                       err);
  if (err != transport::PipeErr::Success) {
    std::cout << "Error on creating server_pipe";
    return;
  }
  auto stream = server_pipe.StartStream();
  if (!stream) {
    std::cout << "Error on starting pipe stream: "
              << static_cast<int>(stream.error()) << '\n';
    return;
  }
  while (true) {
    auto md = handlers::ReadMetadata(*stream);
    if (!md) {
      // Reopening a stream to block
      if (md.error() == handlers::ErrReadMd::Eof) {
        auto new_stream = server_pipe.StartStream();
        if (!new_stream) {
          std::cout << "Error on restarting pipe stream: "
                    << static_cast<int>(stream.error()) << '\n';
          return;
        }
        *stream = std::move(*new_stream);
      }
      if (md.error() == handlers::ErrReadMd::SystemError) {
        std::cout << "System error on reading metadata" << '\n';
      }
      continue;
    }
    switch (md->msg_type) {
    case kConnectionMsgID:
      std::cout << "Server is answering connection request\n";
      handlers::HandleRequest<messenger::ConnectResponce,
                              messenger::ConnectMessage>(
          *stream, *md, [&messeging_service](auto req) -> auto {
            return messeging_service.CreateConnection(req);
          });
      break;
      // idk
    case kDisconnectMsgID:
      std::cout << "Server is requested to disconnect\n";
      handlers::HandleRequest<messenger::DisconnectResponce,
                              messenger::DisconnectMessage>(
          *stream, *md, [&messeging_service](auto req) -> auto {
            return messeging_service.CloseConnection(req);
          });
      break;
    case kSendMsgID:
      std::cout << "Server is requested to send message\n";
      handlers::HandleRequest<messenger::SendResponce, messenger::SendMessage>(
          *stream, *md, [&messeging_service](auto req) -> auto {
            return messeging_service.SendMessage(req);
          });

      break;
    }
  }
}
