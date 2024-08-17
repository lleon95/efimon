/**
 * @copyright Copyright (c) 2024. ZeroMQ Examples
 */

#include <json/json.h>

#include <iostream>
#include <string>
#include <zmqpp/zmqpp.hpp>

using namespace std;  // NOLINT

int main(int, char **) {
  const string endpoint = "tcp://localhost:5550";

  Json::Value root;
  Json::StreamWriterBuilder wbuilder;

  root["root"] = "Hello World";
  root["port"] = 5550;
  root["stop"] = true;

  // initialize the 0MQ context
  zmqpp::context context;

  // generate a push socket
  zmqpp::socket_type type = zmqpp::socket_type::req;
  zmqpp::socket socket(context, type);

  // open the connection
  cout << "Connecting to hello world server…" << endl;
  socket.connect(endpoint);
  while (true) {
    zmqpp::message message;
    std::string s;
    std::getline(cin, s);
    std::cout << "[Sent Message]: " << s << std::endl;
    message << s;
    socket.send(message);
    string buffer;
    socket.receive(buffer);
    std::cout << "[Received Message]: " << buffer << std::endl;
  }

  // message << Json::writeString(wbuilder, root);

  // Samples:
  // {"transaction": "system", "state": true}
  // {"transaction": "process", "pid": 27374, "state": false}
}
