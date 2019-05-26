#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "game.h"
#include "socketstream/socketstream.hh"

using namespace std;
using namespace swoope;

class Server {
 private:
  int n;
  int port;
  socketstream listener;
  socketstream* clients;
  int playerNumber = 0;

 public:
  Server(int port, int n) {
    this->port = port;
    this->n = n;
  }
  bool validPlayerType(string a) {
    if (a == "human" || a == "alien" || a == "zombie" || a == "doggo")
      return true;
    return false;
  }
  void initialize() {
    listener.open(port, n - 1);

    cout << "Waiting for connection...\n";
    clients = new socketstream[n];

    for (int i = 1; i < n; i++) {
      cout << "Waiting for P" << (i + 1) << "...\n";
      listener.accept(clients[i]);
      clients[i] << i << endl;
      cout << "P" << (i + 1) << " ready.\n";
    }
    cout << "Ready" << endl;
    cout << "Pick a player type: " << endl;
    cout << "human" << endl
         << "alien" << endl
         << "doggo" << endl
         << "zombie" << endl;
    string playerType;
    while (!cin.good() || !validPlayerType(playerType)) {
      cin >> playerType;
    }
  };

  class Client {
   private:
    int playerNumber;
    int port;
    string ip;
    socketstream server;
    Player* player;

   public:
    Client(int port, string ip) {
      this->port = port;
      this->ip = ip;
    }
    bool initialize() {
      server.open(ip, port);
      if (!server.good()) return false;
      server >> playerNumber;
      cout << "Your player number is: " << playerNumber + 1 << endl;
      bool ready = false;
      cout << "Waiting for other players" << endl;
      server >> ready;
      cout << "Ready" << endl;
    }
  };

  int main(int argc, char* argv[]) {
    int p = atoi(argv[1]);
    if (p < 1024 || p > 65535) {
      cout << "ERROR: Port must be within range of 1024-65535" << endl;
      return 0;
    }
    if (argc == 2) {
      int n;
      while (!cin.good() || n < 2 || n > 6) {
        cin >> n;
      }
      Server* server = new Server(p, n);
      server->initialize();
    } else if (argc == 3) {
      string ip = argv[2];
      Client* client = new Client(p, ip);
      if (!client->initialize()) {
        cout << "ERROR: IP INVALID" << endl;
        return 0;
      }
    }
  }
