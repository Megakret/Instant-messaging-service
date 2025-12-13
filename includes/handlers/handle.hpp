#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <string.h>

#include <transport/PipeTransport.hpp>

namespace handlers {

struct Metadata {
  int32_t msg_type;
  int32_t length;
};
struct ResponseMetadata {
  int32_t length;
};
template <typename ProtoMessage, typename MetadataT>
transport::ErrSend
BindMetadataAndSend(const ProtoMessage &msg, const MetadataT &metadata,
                    const transport::PipeTransport &user_transport) {
  std::vector<char> user_buf(sizeof(MetadataT) + msg.ByteSizeLong());
  std::span<char> user_buf_span(user_buf);
  std::span<char> metadata_buf = user_buf_span.subspan(0, sizeof(MetadataT));
  std::span<char> msg_buf =
      user_buf_span.subspan(sizeof(MetadataT), msg.ByteSizeLong());
  memcpy(metadata_buf.data(), &metadata, sizeof(MetadataT));
  if (!msg.SerializeToArray(msg_buf.data(), msg_buf.size())) {
    std::cout << "Failed to write msg due to serializing error\n";
  };
  auto err = user_transport.Send(user_buf_span);
  if (err.error_code != transport::kSuccess) {
    return err;
  }
  return transport::ErrSend{transport::kSuccess};
}

template <typename ProtoResponse, typename ProtoRequest>
void HandleRequest(transport::PipeStream &server_stream, const Metadata &md,
                   std::function<ProtoResponse(ProtoRequest)> handler) {
  ProtoRequest request;
  std::string buf(md.length, '\0');
  auto [read_bytes, err] = server_stream.Receive(buf);
  if (err.error_code != transport::kSuccess) {
    std::cout << "Error occured on receiving actual message: " << err.error_code
              << "Errno: " << strerror(err._errno);
  }
  if (!request.ParseFromString(buf)) {
    std::cout << "Failed to accept connection due to parsing error\n";
    return;
  }
  transport::ErrCreation err_creation;
  transport::PipeTransport user_transport(request.pipe_path(), transport::Write,
                                          err_creation);
  auto response = handler(request);
  ResponseMetadata resp_md{static_cast<int32_t>(response.ByteSizeLong())};
  auto err_send = BindMetadataAndSend(response, resp_md, user_transport);
  if (err_send.error_code != transport::kSuccess) {
    std::cout << "Error on sending a message: " << strerror(err_send._errno)
              << '\n';
  }
}
struct ErrReadMd {
  enum Codes {
    Success,
    PartialRead,
    SystemError,
  };
  Codes error_code;
  int _errno;
};
std::pair<Metadata, ErrReadMd> ReadMetadata(transport::PipeStream &stream);
} // namespace handlers
