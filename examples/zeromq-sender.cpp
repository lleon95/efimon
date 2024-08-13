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
  int request_nbr;
  for (request_nbr = 0; request_nbr != 1; request_nbr++) {
    // send a message
    cout << "Sending Hello " << request_nbr << "…" << endl;
    zmqpp::message message;
    // compose a message from a string and a number
    message << Json::writeString(wbuilder, root);
    socket.send(message);
    string buffer;
    socket.receive(buffer);

    cout << "Received World " << request_nbr << endl;
  }
}
