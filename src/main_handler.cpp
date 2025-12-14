#include <iostream>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <handlers/messenger_service.hpp>
#include <protos/main.pb.h>

void main_handler_loop() {
  handlers::MessegingService messeging_service;
  transport::ErrCreation err;
  transport::PipeTransport server_pipe(std::string(kAcceptingPipePath),
                                       transport::Read | transport::Create,
                                       err);
  if (err.error_code != transport::kSuccess) {
    std::cout << "Error on creating server_pipe: " << strerror(err._errno);
    return;
  }
  auto [stream, err_stream] = server_pipe.StartStream();
  if (err_stream.error_code != transport::kSuccess) {
    std::cout << "Error on starting pipe stream: " << err_stream.error_code
              << ' ' << strerror(err_stream._errno) << '\n';
    return;
  }
  while (true) {
    auto [md, err_md] = handlers::ReadMetadata(stream);
    if (err_md.error_code != handlers::ErrReadMd::Success) {
      // Reopening a stream to block
      if (err_md.error_code == handlers::ErrReadMd::Eof) {
        auto [new_stream, err_stream] = server_pipe.StartStream();
        stream = std::move(new_stream);
        if (err_stream.error_code != transport::kSuccess) {
          std::cout << "Error on restarting pipe stream: "
                    << err_stream.error_code << ' '
                    << strerror(err_stream._errno) << '\n';
          return;
        }
        continue;
      }
      if (err_md.error_code == handlers::ErrReadMd::SystemError) {
        std::cout << "Error on reading metadata: " << strerror(err_md._errno)
                  << '\n';
      }
    }
    switch (md.msg_type) {
    case kConnectionMsgID:
      std::cout << "Server is answering connection request\n";
      handlers::HandleRequest<messenger::ConnectResponce,
                              messenger::ConnectMessage>(
          stream, md, [&messeging_service](auto req) -> auto {
            return messeging_service.CreateConnection(req);
          });
      break;
      // idk
    case kDisconnectMsgID:
      std::cout << "Server is requested to disconnect\n";
      handlers::HandleRequest<messenger::DisconnectResponce,
                              messenger::DisconnectMessage>(
          stream, md, [&messeging_service](auto req) -> auto {
            return messeging_service.CloseConnection(req);
          });
      break;
    case kSendMsgID:
      std::cout << "Server is requested to send message\n";
      handlers::HandleRequest<messenger::SendResponce, messenger::SendMessage>(
          stream, md, [&messeging_service](auto req) -> auto {
            return messeging_service.SendMessage(req);
          });

      break;
    }
  }
}
