#include <fstream>
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
  while (true) {
		auto [stream, err_stream] = server_pipe.StartStream();
		if(err_stream.error_code != transport::kSuccess){
			std::cout << "Error on starting pipe stream: " << strerror(errno) << '\n';
			continue;
		}
    auto [md, err_md] = handlers::ReadMetadata(stream);
    if (err_md.error_code != handlers::ErrReadMd::Success) {
      if (err_md.error_code == handlers::ErrReadMd::SystemError) {
        std::cout << "Error on reading metadata: " << strerror(err_md._errno)
                  << '\n';
      }
    }
    switch (md.msg_type) {
    case kConnectionMsgID:
      handlers::HandleRequest<messenger::ConnectResponce,
                              messenger::ConnectMessage>(
          stream, md, [&messeging_service](auto req) -> auto {
            return messeging_service.CreateConnection(req);
          });
      break;
      // idk
    case kDisconnectMsgID:
      handlers::HandleRequest<messenger::DisconnectResponce,
                              messenger::DisconnectMessage>(
          stream, md, [&messeging_service](auto req) -> auto {
            return messeging_service.CloseConnection(req);
          });
      break;
    case kSendMsgID:
      handlers::HandleRequest<messenger::SendResponce, messenger::SendMessage>(
          stream, md, [&messeging_service](auto req) -> auto {
            return messeging_service.SendMessage(req);
          });

      break;
    }
  }
}
