#include <messenger_service.hpp>

messenger::ConnectResponce
MessegingService::CreateConnection(const messenger::ConnectMessage &msg) {
  messenger::ConnectResponce responce;
  responce.set_status(messenger::ConnectResponce::OK);
  responce.set_verbose("Hello, world");
  responce.set_reading_pipe_path("unknown for now");
  return responce;
}

messenger::DisconnectResponce MessegingService::CloseConnection(const messenger::DisconnectMessage& msg){
	messenger::DisconnectResponce responce;
	responce.set_status(messenger::DisconnectResponce::OK);
	responce.set_verbose("Bye, world\n");
	return responce;
}
