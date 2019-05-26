#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "game.h"
#include "socketstream/socketstream.hh"

using namespace std;
using namespace swoope;

class ICommand {
   public:
    virtual void execute() = 0;
};

enum class Commands {
    InputPlayerType = 'a',
    InputTeamNumber = 'b'
};

char commandToChar(Commands c) {
    return static_cast<char>(c);
}

Commands charToCommand(char c) {
    return static_cast<Commands>(c);
}

class Common {
   protected:
    int n;
    int port;
    int playerNumber = 0;
    int teamNumber;
    string playerType;
    Player* player;

   public:
    bool validPlayerType(string a) {
        if (a == "human" || a == "alien" || a == "zombie" || a == "doggo")
            return true;
        return false;
    }
    void inputPlayerType() {
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
    }
    void inputTeamNumber() {
        cout << "Input your team number from 1-" << n << endl;
        while (!cin.good() || teamNumber < 1 || teamNumber > n) {
            cin >> teamNumber;
        }
    }
};

class Server : public Common {
   private:
    socketstream listener;
    socketstream* clients;
    Game* game;

   public:
    Server(int port, int n) {
        this->port = port;
        this->n = n;
    }
    void initialize() {
        vector<string> playerTypes;
        vector<int> teamNumbers;
        teamNumbers.push_back(1);
        listener.open(port, n - 1);

        cout << "Waiting for connection...\n";
        clients = new socketstream[n];

        for (int i = 1; i < n; i++) {
            cout << "Waiting for P" << (i + 1) << "...\n";
            listener.accept(clients[i]);
            clients[i] << i << endl;
            cout << "P" << (i + 1) << " ready.\n";
        }
        inputPlayerType();
        playerTypes.push_back(playerType);
        for (int i = 1; i < n; i++) {
            clients[i] << commandToChar(Commands::InputPlayerType) << endl;
            string s;
            getline(clients[i], s);
            playerTypes.push_back(s);
        }
        for (int i = 1; i < n; i++) {
            clients[i] << commandToChar(Commands::InputTeamNumber) << endl;
            int t;
            clients[i] >> t;
            clients[i].ignore();
            teamNumbers.push_back(t);
        }
        game = new Game(teamNumbers, playerTypes);
    }
    void start() {
        }
};

class Client : public Common {
   private:
    string ip;
    socketstream server;

   public:
    Client(int port, string ip) {
        this->port = port;
        this->ip = ip;
    }
    bool initialize() {
        char cmd;
        server.open(ip, port);
        if (!server.good()) return false;
        server >> playerNumber;
        server.ignore();
        cout << "Your player number is: " << playerNumber + 1 << endl;
        bool ready = false;
        cout << "Waiting for other players" << endl;
        server >> ready;
        server.ignore();
        cout << "Ready" << endl;
        server >> cmd;
        server.ignore();
        if (charToCommand(cmd) == Commands::InputPlayerType) {
            inputPlayerType();
            server << playerType << endl;
        }
        server >> cmd;
        server.ignore();
        if (charToCommand(cmd) == Commands::InputTeamNumber) {
            inputTeamNumber();
            server << playerType << endl;
        }
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
