#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <ncurses.h>
#undef OK
#include <sstream>
#include <string>
#include <thread>

#include <config.hpp>
#include <handlers/handle.hpp>
#include <protos/main.pb.h>

const std::string_view kClientPipeDir = "/tmp/clients/";
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
const auto kCurrentZone = std::chrono::current_zone();
enum class SendToServerStatus {
  Success,
  TransportErr,
  SerializationErr,
  Error,
};
template <typename ProtoMessage>
SendToServerStatus SendMessage(const ProtoMessage& msg, int64_t request_id,
                               const transport::PipeTransport& server_pipe) {
  Metadata md{request_id, static_cast<int64_t>(msg.ByteSizeLong())};
  auto err = handlers::BindMetadataAndSend(msg, md, server_pipe);
  switch (err) {
  case handlers::BindMdAndSendErr::Success:
    return SendToServerStatus::Success;
  case handlers::BindMdAndSendErr::TransportErr:
    std::cerr << "Failed to send to server\n";
    return SendToServerStatus::TransportErr;
  case handlers::BindMdAndSendErr::SerializationErr:
    std::cerr << "Failed to send to server\n";
    return SendToServerStatus::SerializationErr;
  }
  return SendToServerStatus::Error;
}
enum class GetFromPipeErr {
  SystemError,
  Eof,
  ParseError,
};
void PrintErr(GetFromPipeErr err) {
  if (err == GetFromPipeErr::SystemError) {
    std::cerr << "Error when reading from pipe: SystemError\n";
    return;
  }
  if (err == GetFromPipeErr::Eof) {
    std::cerr << "Error when reading from pipe: EOF\n";
    return;
  }
  if (err == GetFromPipeErr::ParseError) {
    std::cerr << "Error when reading from pipe: ParseError\n";
    return;
  }
  std::cerr << "Error when reading from pipe: Unknown\n";
}
template <typename ProtoResponse>
std::expected<ProtoResponse, GetFromPipeErr>
GetFromStream(const transport::PipeStream& stream) {
  ProtoResponse response;
  std::string buf(sizeof(ResponseMetadata), '\0');
  auto read_bytes = stream.Receive(buf);
  auto metadata = reinterpret_cast<const ResponseMetadata*>(buf.c_str());
  std::string msg_buf(metadata->length, '\0');
  auto read_bytes_msg = stream.Receive(msg_buf);
  if (!read_bytes_msg) {
    auto err = read_bytes_msg.error();
    std::cerr << "Error on read from pipe: " << static_cast<int>(err) << '\n';
    return std::unexpected(GetFromPipeErr::SystemError);
  }
  if (read_bytes_msg == 0) {
    return std::unexpected(GetFromPipeErr::Eof);
  }
  if (!response.ParseFromString(msg_buf)) {
    return std::unexpected(GetFromPipeErr::ParseError);
  }
  return response;
}
template <typename ProtoResponse>
std::expected<ProtoResponse, GetFromPipeErr>
GetFromPipe(const transport::PipeTransport& pipe) {
  auto stream = pipe.StartStream();
  if (!stream) {
    return std::unexpected(GetFromPipeErr::SystemError);
  }
  return GetFromStream<ProtoResponse>(*stream);
}

struct Message {
  std::string sender;
  std::string content;
  std::chrono::system_clock::time_point timestamp;
};

class User {
private:
  std::mutex user_mu_;
  std::string login_;
  transport::PipeTransport resp_pipe_;
  transport::PipeTransport server_pipe_;
  std::unique_ptr<transport::PipeTransport> recv_pipe_ = nullptr;
  std::function<void(Message)> callback_;

public:
  User(std::string login, std::function<void(Message)> callback)
      : login_(login), resp_pipe_(std::string(kClientPipeDir) + login,
                                  transport::Read | transport::Create),
        callback_(callback),
        server_pipe_(std::string(kAcceptingPipePath), transport::Write) {

    {
      messenger::ConnectMessage req;
      req.set_login(login);
      req.set_pipe_path(resp_pipe_.GetPath());
      auto status = SendMessage<messenger::ConnectMessage>(
          req, kConnectionMsgID, server_pipe_);
      if (status != SendToServerStatus::Success) {
        return;
      }
    }
    {
      auto msg(GetFromPipe<messenger::ConnectResponce>(resp_pipe_));
      if (!msg) {
        PrintErr(msg.error());
        return;
      }
      if (msg->status() == messenger::ConnectResponce::ERROR) {
        std::cerr << "Error from server on connecting\n";
        return;
      }
      transport::PipeErr err;
      std::cerr << msg->reading_pipe_path() << '\n';
      recv_pipe_ = std::make_unique<transport::PipeTransport>(
          msg->reading_pipe_path(), transport::Read, err);
      if (err != transport::PipeErr::Success) {
        std::cerr << "Error on creating recv pipe: " << static_cast<int>(err)
                  << '\n';
        return;
      }
    }
  }
  ~User() {
    {
      messenger::DisconnectMessage req;
      req.set_login(login_);
      req.set_pipe_path(resp_pipe_.GetPath());
      auto status = SendMessage<messenger::DisconnectMessage>(
          req, kConnectionMsgID, server_pipe_);
      if (status != SendToServerStatus::Success) {
        std::cerr << "Failed to connect\n";
        return;
      }
    }
    {
      auto msg(GetFromPipe<messenger::DisconnectResponce>(resp_pipe_));
      if (!msg) {
        PrintErr(msg.error());
        return;
      }
      if (msg->status() == messenger::DisconnectResponce::ERROR) {
        std::cerr << "Error from server  on disconnecting\n";
      }
    }
  }

  void SendToUser(const std::string& receiver, const std::string& message) {
    std::lock_guard<std::mutex> lock(user_mu_);
    // In real implementation, send over network
    messenger::SendMessage req;
    req.set_sender_login(login_);
    req.set_receiver_login(receiver);
    req.set_message(message);
    req.set_pipe_path(resp_pipe_.GetPath());
    auto status = SendMessage(req, kSendMsgID, server_pipe_);
    if (status != SendToServerStatus::Success) {
      return;
    }
    {
      auto msg(GetFromPipe<messenger::SendResponce>(resp_pipe_));
      if (!msg) {
        PrintErr(msg.error());
        return;
      }
      if (msg->status() == messenger::SendResponce::ERROR) {
        std::cerr << "Error from server on sending message\n";
      }
    }
  }

  void SendDelayedToUser(const std::string& receiver,
                         const std::string& message,
                         std::chrono::system_clock::time_point time) {
    std::lock_guard<decltype(user_mu_)> lock(user_mu_);
    // In real implementation, schedule for later
    messenger::SendPostponedRequest req;
    req.set_pipe_path(resp_pipe_.GetPath());
    auto msg = req.mutable_msg();
    msg->set_sender_login(login_);
    msg->set_message(message);
    msg->set_time(
        static_cast<int32_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                 time.time_since_epoch())
                                 .count()));
    req.set_receiver(receiver);
    auto status = SendMessage(req, kPostponeMsgID, server_pipe_);
    if (status != SendToServerStatus::Success) {
      return;
    }
    {
      auto msg(GetFromPipe<messenger::SendPostponedResponce>(resp_pipe_));
      if (msg->status() == messenger::SendPostponedResponce::ERROR) {
        std::cerr << "Error on sending postponed message\n";
      }
    }
  }

  void RunReceiverLoop() {
    if (recv_pipe_ == nullptr) {
      return;
    }
    while (true) {
      auto stream = recv_pipe_->StartStream();
      if (!stream) {
        std::cerr << "Failed to create receiving stream\n";
        return;
      }
      while (true) {
        auto msg(GetFromStream<messenger::MessageForUser>(*stream));
        std::cerr << "Got message\n";
        if (!msg) {
          auto err = msg.error();
          if (err == GetFromPipeErr::SystemError) {
            std::cerr << "Error on receiving message from pipe: ";
            PrintErr(err);
            continue;
          }
          if (err == GetFromPipeErr::Eof) {
            break;
          }
        }
        callback_(Message{std::move(msg->sender_login()),
                          std::move(msg->message()),
                          std::chrono::system_clock::time_point(
                              std::chrono::seconds(msg->time()))});
      }
    }
  }
};

class ChatApp {
private:
  WINDOW* history_win = nullptr;
  WINDOW* input_win = nullptr;
  std::unique_ptr<User> user_ = nullptr;
  std::mutex ui_mutex;
  std::string time_to_string(const std::chrono::system_clock::time_point& tp) {
    return std::format("{:%F %H-%M}", tp);
  }

  void print_message_to_history(const Message& msg) {
    std::lock_guard<decltype(ui_mutex)> lock(ui_mutex);
    std::string formatted = "@" + msg.sender + " [" +
                            time_to_string(msg.timestamp) + "] " + msg.content;
    wprintw(history_win, "%s\n", formatted.c_str());
    wrefresh(history_win);
  }

  std::string get_input_line(WINDOW* win) {
    char buffer[1024];
    echo();
    wgetnstr(win, buffer, sizeof(buffer) - 1);
    noecho();
    return std::string(buffer);
  }

  void clear_input_window() {
    werase(input_win);
    box(input_win, 0, 0);
    wrefresh(input_win);
  }

  void run_input_loop() {
    wprintw(input_win, "Enter your login: ");
    std::string login = get_input_line(input_win);
    clear_input_window();
    user_ = std::make_unique<User>(login, [this](const Message& msg) {
      this->print_message_to_history(msg);
    });
    // Start receiver thread
    std::thread receiver_thread([this]() { this->user_->RunReceiverLoop(); });
    receiver_thread.detach();
    while (true) {
      mvwprintw(
          input_win, 1, 2,
          "Select the message you want to send (1: instant, 2: delayed): ");
      wrefresh(input_win);

      std::string choice = get_input_line(input_win);
      clear_input_window();
      // Instant message
      if (choice == "1") {
        mvwprintw(input_win, 1, 2, "Input the receiver login: ");
        wrefresh(input_win);
        std::string receiver = get_input_line(input_win);

        clear_input_window();
        mvwprintw(input_win, 1, 2, "Input the message: ");
        wrefresh(input_win);
        std::string message = get_input_line(input_win);

        user_->SendToUser(receiver, message);
        clear_input_window();
        // Delayed message
      } else if (choice == "2") {
        mvwprintw(input_win, 1, 2, "Input the receiver login: ");
        wrefresh(input_win);
        std::string receiver = get_input_line(input_win);

        clear_input_window();
        mvwprintw(input_win, 1, 2, "Input the message: ");
        wrefresh(input_win);
        std::string message = get_input_line(input_win);

        clear_input_window();
        mvwprintw(input_win, 1, 2, "Input date (YYYY-MM-DD hh-mm): ");
        wrefresh(input_win);
        std::string time_str = get_input_line(input_win);

        std::chrono::local_seconds time_to_send;
        std::istringstream istream{time_str};
        istream >> std::chrono::parse("%F %R", time_to_send);
        // TODO: failed to parse
        if (istream.fail()) {
          mvwprintw(input_win, 1, 2,
                    "Invalid time format. Press the key to continue...");
          wrefresh(input_win);
          getch();
          clear_input_window();
        }
        user_->SendDelayedToUser(receiver, message,
                                 kCurrentZone->to_sys(time_to_send));
        clear_input_window();
      } else {
        mvwprintw(input_win, 1, 2,
                  "Invalid choice. Press any key to continue...");
        wrefresh(input_win);
        getch();
        clear_input_window();
      }
    }
  }

public:
  void run() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);

    // Create windows
    history_win = newwin(height - 5, width, 0, 0);
    input_win = newwin(5, width, height - 5, 0);

    scrollok(history_win, TRUE);
    box(input_win, 0, 0);
    wrefresh(history_win);

    // Run input loop in main thread
    run_input_loop();

    endwin();
  }
};

int main() {
  ChatApp app;
  app.run();
  return 0;
}
