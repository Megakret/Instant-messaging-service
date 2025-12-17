#include <iostream>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <handlers/messenger_service.hpp>
#include <handlers/postpone_handlers.hpp>
#include <postpone_service.hpp>
#include <protos/main.pb.h>
#include <thread.hpp>

void main_handler_loop(std::chrono::seconds postpone_timeout) {
  UserStorage users;
  std::mutex users_mu;
  handlers::MessegingService messeging_service(users, users_mu);
  PostponeService postponer(users, users_mu);
  os::Thread t;
  auto postpone_runner =
      std::function<void()>([&postponer, postpone_timeout]() {
        postponer.StartSendSchedule(postpone_timeout);
      });
  t.RunLambda(&postpone_runner);
  t.Detach();
  handlers::PostponeMessageHandler postpone_handler(postponer);
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

    case kPostponeMsgID:
      std::cout << "Server is requested to postpone message\n";
      handlers::HandleRequest<messenger::SendPostponedResponce,
                              messenger::SendPostponedRequest>(
          *stream, *md, [&postpone_handler](auto req) -> auto {
            return postpone_handler(req);
          });

      break;
    }
  }
}
