/**
 * @copyright Copyright (c) 2024. ZeroMQ Examples
 */

#include <json/json.h>

#include <iostream>
#include <string>
#include <zmq.hpp>

using namespace std;  // NOLINT

int main(int, char **) {
  const string endpoint = "tcp://localhost:5550";

  Json::Value root;
  Json::StreamWriterBuilder wbuilder;

  root["root"] = "Hello World";
  root["port"] = 5550;
  root["stop"] = true;

  // initialize the 0MQ context
  zmq::context_t context;

  // generate a push socket
  auto type = zmq::socket_type::req;
  zmq::socket_t socket(context, type);

  // open the connection
  cout << "Connecting to hello world server…" << endl;
  socket.connect(endpoint);
  while (true) {
    std::string s;
    std::getline(cin, s);
    std::cout << "[Sent Message]: " << s << std::endl;
    zmq::message_t message(s);
    socket.send(message, zmq::send_flags::none);
    zmq::message_t buffer;
    auto res = socket.recv(buffer, zmq::recv_flags::none);
    std::cout << "[Received Message with status " << res.value()
              << "]: " << buffer.str() << std::endl;
  }

  // message << Json::writeString(wbuilder, root);

  // Samples:
  // {"transaction": "system", "state": true}
  // {"transaction": "process", "pid": 27374, "state": false}
}
